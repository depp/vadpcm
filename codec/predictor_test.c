// Copyright 2023 Dietrich Epp.
// This file is part of VADPCM. VADPCM is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "codec/predictor.h"
#include "codec/autocorr.h"
#include "codec/random.h"
#include "codec/test.h"

#include <math.h>
#include <stdio.h>

#pragma GCC diagnostic ignored "-Wdouble-promotion"

void test_autocorr(void) {
    // Check that the error for a set of coefficients can be calculated using
    // the autocorrelation matrix.
    static const float coeff[2] = {0.5f, 0.25f};

    uint32_t state = 1;
    int failures = 0;
    for (int test = 0; test < 10; test++) {
        // Generate some random data.
        int16_t data[kVADPCMFrameSampleCount * 2];
        for (int i = 0; i < kVADPCMFrameSampleCount * 2; i++) {
            data[i] = 0;
        }
        for (int i = 0; i <= 4; i++) {
            int n = (kVADPCMFrameSampleCount * 2) >> i;
            int m = 1 << i;
            for (int j = 0; j < n; j++) {
                // Random number in -2^13..2^13-1.
                int s = (int)(state >> 19) - (1 << 12);
                state = vadpcm_rng(state);
                for (int k = 0; k < m; k++) {
                    data[j * m + k] += s;
                }
            }
        }

        // Get the autocorrelation.
        float corr[2][6];
        vadpcm_autocorr(2, corr, data);

        // Calculate error directly.
        float s1 = (float)data[kVADPCMFrameSampleCount - 2] * (1.0f / 32768.0f);
        float s2 = (float)data[kVADPCMFrameSampleCount - 1] * (1.0f / 32768.0f);
        float error = 0;
        for (int i = 0; i < kVADPCMFrameSampleCount; i++) {
            float s = data[kVADPCMFrameSampleCount + i] * (1.0 / 32768.0);
            float d = s - coeff[1] * s1 - coeff[0] * s2;
            error += d * d;
            s1 = s2;
            s2 = s;
        }

        // Calculate error from autocorrelation matrix.
        float eval = vadpcm_eval(corr[1], coeff);

        if (fabsf(error - eval) > (error + eval) * 1.0e-4f) {
            fprintf(stderr,
                    "test_autororr case %d: "
                    "error = %f, eval = %f, relative error = %f\n",
                    test, error, eval, fabsf(error - eval) / (error + eval));
            failures++;
        }
    }
    if (failures > 0) {
        fprintf(stderr, "test_autocorr failures: %d\n", failures);
        test_failure_count++;
    }
}

void test_solve(void) {
    // Check that vadpcm_solve minimizes vadpcm_eval.
    static const double dcorr[][6] = {
        // Simple positive definite matrixes.
        {4.0, 1.0, 5.0, 2.0, 3.0, 6.0},
        {4.0, -1.0, 5.0, -2.0, -3.0, 6.0},
        {4.0, 1.0, 6.0, 2.0, 3.0, 5.0},
        // Singular matrixes.
        {1.0, 0.5, 1.0, 0.0, 0.0, 0.0},
        {1.0, 0.0, 0.0, 0.5, 0.0, 1.0},
        {1.0, 0.25, 2.0, 0.25, 2.0, 2.0},
        // Zero submatrix.
        {1.0, 0.0, 0.0, 0.0, 0.0, 0.0},
        // Zero.
        {0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    };
    static const float offset[][2] = {
        {1.0f, 0.0f},
        {0.0f, 1.0f},
        {-1.0f, 0.0f},
        {0.0f, -1.0f},
    };
    static const float offset_amt = 0.01f;
    int failures = 0;
    for (size_t test = 0; test < sizeof(dcorr) / sizeof(*dcorr); test++) {
        double dcoeff[2];
        vadpcm_solve(dcorr[test], dcoeff);
        float corr[6];
        float coeff[2];
        for (int i = 0; i < 6; i++) {
            corr[i] = (float)dcorr[test][i];
        }
        for (int i = 0; i < 2; i++) {
            coeff[i] = (float)dcoeff[i];
        }
        // Test that this is a local minimum.
        float error = vadpcm_eval(corr, coeff);
        if (error < 0.0f) {
            fprintf(stderr, "test_solve case %zu: negative error\n", test);
            failures++;
            continue;
        }
        float min_error = error - error * (1.0f / 65536.0f);
        for (size_t off = 0; off < sizeof(offset) / sizeof(*offset); off++) {
            float ocoeff[2];
            for (int i = 0; i < 2; i++) {
                ocoeff[i] = coeff[i] + offset[off][i] * offset_amt;
            }
            float oerror = vadpcm_eval(corr, ocoeff);
            if (oerror < min_error) {
                fprintf(stderr, "test_solve case %zu: not a local minimum\n",
                        test);
                failures++;
            }
        }
        // Check that eval_solved() gives us the same result as eval().
        double error2 = vadpcm_eval_solved(dcorr[test], dcoeff);
        if (fabs(error2 - (double)error) > (double)error * (1.0 / 65536.0)) {
            fprintf(stderr,
                    "test_solve case %zu: eval_solved() is incorrect\n"
                    "\teval_solved = %f\n"
                    "\teval        = %f\n",
                    test, error2, (double)error);
            failures++;
            continue;
        }
    }
    if (failures > 0) {
        fprintf(stderr, "test_solve failures: %d\n", failures);
        test_failure_count++;
    }
}
