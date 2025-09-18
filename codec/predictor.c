// Copyright 2023 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "codec/predictor.h"

#include "codec/vadpcm.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// A record of the predictor error if a frame is encoded with a given predictor.
struct vadpcm_error_record {
    int predictor;
    float error;
};

// Scratch data for a frame when assigning predictors.
struct vadpcm_scratch {
    struct vadpcm_error_record p[2];
};

enum {
    // Iterations for predictor assignment.
    kVADPCMIterations = 20,
};

float vadpcm_eval(const float corr[restrict static 6],
                  const float coeff[restrict static 2]);

void vadpcm_best_error(size_t frame_count, const float (*restrict corr)[6],
                       float *restrict best_error) {
    for (size_t frame = 0; frame < frame_count; frame++) {
        double fcorr[6];
        for (int i = 0; i < 6; i++) {
            fcorr[i] = (double)corr[frame][i];
        }
        double coeff[2];
        vadpcm_solve(fcorr, coeff);
        float fcoeff[2] = { (float)coeff[0], (float)coeff[1] };
        best_error[frame] = (float)vadpcm_eval(corr[frame], fcoeff);
    }
}

// Get the mean autocorrelation matrix for each predictor. If the predictor for
// a frame is out of range, that frame is ignored.
void vadpcm_meancorrs(size_t frame_count, int predictor_count,
                      const float (*restrict corr)[6],
                      const uint8_t *restrict predictors,
                      double (*restrict pcorr)[6], int *restrict count) {
    for (int i = 0; i < predictor_count; i++) {
        count[i] = 0;
        for (int j = 0; j < 6; j++) {
            pcorr[i][j] = 0.0;
        }
    }
    for (size_t frame = 0; frame < frame_count; frame++) {
        int predictor = predictors[frame];
        if (predictor < predictor_count) {
            count[predictor]++;
            // REVIEW: This is naive summation. Is that good enough?
            for (int j = 0; j < 6; j++) {
                pcorr[predictor][j] += (double)corr[frame][j];
            }
        }
    }
    for (int i = 0; i < predictor_count; i++) {
        if (count[i] > 0) {
            double a = 1.0 / count[i];
            for (int j = 0; j < 6; j++) {
                pcorr[i][j] *= a;
            }
        }
    }
}

/* Map the 3x3 upper-triangular autocorrelation (flattened as corr[6]) to
 * Yule–Walker lags r(0), r(1), r(2) for a (nearly) Toeplitz structure.
 *
 * Layout in encode.c:
 *   // [0 1 3]
 *   // [_ 2 4]
 *   // [_ _ 5]
 * and with x0 = s[n], x1 = s[n-1], x2 = s[n-2]:
 *   corr[0] = Σ x0 x0   ≈ r(0)
 *   corr[1] = Σ x1 x0   ≈ r(1)
 *   corr[2] = Σ x1 x1   ≈ r(0)
 *   corr[3] = Σ x2 x0   ≈ r(2)
 *   corr[4] = Σ x2 x1   ≈ r(1)
 *   corr[5] = Σ x2 x2   ≈ r(0)
 *
 * We average duplicates to enforce Toeplitz consistency over a finite frame.
 */
static inline void vadpcm_corr_to_rxx_yw(const double corr[restrict static 6],
                                         double rxx[restrict static 3])
{
    /* r0: average of the three diagonal terms (variance at lags 0,1,2) */
    double r0 = (corr[0] + corr[2] + corr[5]) / 3.0;
    /* r1: average of x0-x1 and x1-x2 cross-terms */
    double r1 = (corr[1] + corr[4]) / 2.0;
    /* r2: x0-x2 */
    double r2 = corr[3];

    rxx[0] = r0;
    rxx[1] = r1;
    rxx[2] = r2;
}

/* Solve AR(2) predictor coefficients via Yule–Walker / Levinson–Durbin.
 *
 * Model and notation:
 *   We predict s[n] from its two past samples:
 *       s[n] = a1 s[n-1] + a2 s[n-2] + e[n]
 *
 *   In AR notation:
 *       s[n] + φ1 s[n-1] + φ2 s[n-2] = e[n]
 *   with the relation between parameters:
 *       a_k = -φ_k   (k = 1,2)
 *
 *   Yule–Walker normal equations (Toeplitz):
 *       [ r(0)  r(1) ] [ φ1 ] = [ r(1) ]
 *       [ r(1)  r(0) ] [ φ2 ]   [ r(2) ]
 *
 *   Levinson–Durbin recursion (p = 2):
 *     - Order 1:
 *         k1 = - r(1) / (r(0) + λ)
 *         φ1^(1) = k1
 *         E1 = (r(0) + λ) * (1 - k1^2)
 *     - Order 2:
 *         k2 = - ( r(2) + φ1^(1) r(1) ) / E1
 *         φ1 = φ1^(1) * (1 + k2)
 *         φ2 = k2
 *   Finally:
 *       a1 = -φ1,  a2 = -φ2
 *
 * Inputs:
 *   corr[6]  : upper-triangular 3x3 autocorrelation as produced by vadpcm_autocorr
 * Outputs:
 *   coeff[2] : { a1, a2 } in the same convention used by encode.c
 *
 * Notes:
 *   - A tiny ridge λ is added to r(0) for numerical robustness on short/flat frames.
 *   - Reflection coefficients k1, k2 are softly clamped to keep |k| < 1.
 *   - With exact YW/LD on a valid covariance sequence, stability is guaranteed.
 */
void vadpcm_solve(const double corr[restrict static 6],
                  double coeff[restrict static 2]) {
    double r[3];
    vadpcm_corr_to_rxx_yw(corr, r);

    /* Degenerate case: no variance */
    if (!(r[0] > 0.0)) {
        coeff[0] = 0.0;
        coeff[1] = 0.0;
        return;
    }

    /* Small ridge for conditioning (relative to r0) */
    const double eps_abs = 1e-12;
    double lambda = fabs(r[0]) * 1e-6 + eps_abs;

    double E0 = r[0] + lambda;

    /* Order 1 (compute k1 and φ1^(1)) */
    double k1 = -r[1] / E0;
    if (fabs(k1) >= 0.9999) k1 = copysign(0.9999, k1);

    double phi1_1 = k1;
    double E1 = E0 * (1.0 - k1 * k1);

    /* If we cannot proceed to order 2, degrade to AR(1) */
    if (!(E1 > (1e-9) * E0)) {
        coeff[0] = -phi1_1;
        coeff[1] = 0.0;
        return;
    }

    /* Order 2 (compute k2 and final φ1, φ2) */
    double k2 = -(r[2] + phi1_1 * r[1]) / E1;
    if (fabs(k2) >= 0.9999) k2 = copysign(0.9999, k2);

    double phi1 = phi1_1 * (1.0 + k2);
    double phi2 = k2;

    /* Convert AR φ to predictor a */
    coeff[0] = -phi1;  /* a1 */
    coeff[1] = -phi2;  /* a2 */

    /* Optional tiny shrink if roots are numerically ~1 */
    {
        double a1 = coeff[0], a2 = coeff[1];
        /* Characteristic: z^2 - a1 z - a2 = 0 */
        double disc = a1 * a1 + 4.0 * a2;
        double rmax;
        if (disc >= 0.0) {
            double s = sqrt(disc);
            double r1 = 0.5 * (a1 + s);
            double r2c = 0.5 * (a1 - s);
            rmax = fmax(fabs(r1), fabs(r2c));
        } else {
            /* Complex conjugate: |root| = sqrt(|a2|) */
            rmax = sqrt(fabs(a2));
        }
        if (rmax >= 0.999999) {
            double shrink = 0.999999 / rmax;
            coeff[0] *= shrink;
            coeff[1] *= shrink;
        }
    }
}

// Refine (improve) the existing predictor assignments. Does not assign
// unassigned predictors. Record the amount of error, squared, for each frame.
// Returns the index of an unassigned predictor, or predictor_count, if no
// predictor is unassigned.
static int vadpcm_refine_predictors(size_t frame_count, int predictor_count,
                                    const float (*restrict corr)[6],
                                    float *restrict error,
                                    uint8_t *restrict predictors) {
    // Calculate optimal predictor coefficients for each predictor.
    double pcorr[kVADPCMMaxPredictorCount][6];
    int count[kVADPCMMaxPredictorCount];
    vadpcm_meancorrs(frame_count, predictor_count, corr, predictors, pcorr,
                     count);

    float coeff[kVADPCMMaxPredictorCount][2];
    int active_count = 0;
    for (int i = 0; i < predictor_count; i++) {
        if (count[i] > 0) {
            double dcoeff[2];
            vadpcm_solve(pcorr[i], dcoeff);
            for (int j = 0; j < 2; j++) {
                coeff[active_count][j] = dcoeff[j];
            }
            active_count++;
        }
    }

    // Assign frames to the best predictor for each frame, and record the amount
    // of error.
    int count2[kVADPCMMaxPredictorCount];
    for (int i = 0; i < active_count; i++) {
        count2[i] = 0;
    }
    for (size_t frame = 0; frame < frame_count; frame++) {
        int fpredictor = 0;
        float ferror = 0.0f;
        for (int i = 0; i < active_count; i++) {
            float e = vadpcm_eval(corr[frame], coeff[i]);
            if (i == 0 || e < ferror) {
                fpredictor = i;
                ferror = e;
            }
        }
        predictors[frame] = fpredictor;
        error[frame] = ferror;
        count2[fpredictor]++;
    }
    for (int i = 0; i < active_count; i++) {
        if (count2[i] == 0) {
            return i;
        }
    }
    return active_count;
}

// Find the frame where the error is highest, relative to the best case.
static size_t vadpcm_worst_frame(size_t frame_count,
                                 const float *restrict best_error,
                                 const float *restrict error) {
    float best_improvement = error[0] - best_error[0];
    size_t best_index = 0;
    for (size_t frame = 1; frame < frame_count; frame++) {
        float improvement = error[frame] - best_error[frame];
        if (improvement > best_improvement) {
            best_improvement = improvement;
            best_index = frame;
        }
    }
    return best_index;
}

vadpcm_error vadpcm_assign_predictors(size_t frame_count, int predictor_count,
                                      const float (*restrict corr)[6],
                                      uint8_t *restrict predictors) {
    memset(predictors, 0, predictor_count);
    if (predictor_count <= 1) {
        return 0;
    }
    float *best_error = malloc(frame_count * sizeof(*best_error));
    if (best_error == NULL) {
        return kVADPCMErrMemory;
    }
    float *error = malloc(frame_count * sizeof(*best_error));
    if (error == NULL) {
        free(best_error);
        return kVADPCMErrMemory;
    }
    int unassigned = predictor_count;
    int active_count = 1;
    for (int iter = 0; iter < kVADPCMIterations; iter++) {
        if (unassigned < predictor_count) {
            size_t worst = vadpcm_worst_frame(frame_count, best_error, error);
            predictors[worst] = unassigned;
            if (unassigned >= active_count) {
                active_count = unassigned + 1;
            }
        }
        unassigned = vadpcm_refine_predictors(frame_count, active_count, corr,
                                              error, predictors);
    }
    free(best_error);
    free(error);
    return 0;
}
