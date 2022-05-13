// Copyright 2022 Dietrich Epp.
// This file is part of Skelly 64. Skelly 64 is licensed under the terms of the
// Mozilla Public License, version 2.0. See LICENSE.txt for details.
#include "lib/vadpcm/vadpcm.h"

#include "lib/vadpcm/codebook.h"

#include <math.h>
#include <string.h>

// Autocorrelation is a symmetric 3x3 matrix.
//
// The upper triangle is stored. Indexes:
//
// [0 1 3]
// [_ 2 4]
// [_ _ 5]

// Calculate the autocorrelation matrix for each frame.
static void vadpcm_autocorr(size_t frame_count, float (*restrict corr)[6],
                            const int16_t *restrict src) {
    float x0 = 0.0f, x1 = 0.0f, x2 = 0.0f, m[6];
    size_t frame;
    int i;

    for (frame = 0; frame < frame_count; frame++) {
        for (i = 0; i < 6; i++) {
            m[i] = 0.0f;
        }
        for (i = 0; i < kVADPCMFrameSampleCount; i++) {
            x2 = x1;
            x1 = x0;
            x0 = src[frame * kVADPCMFrameSampleCount + i] * (1.0f / 32768.0f);
            m[0] += x0 * x0;
            m[1] += x1 * x0;
            m[2] += x1 * x1;
            m[3] += x2 * x0;
            m[4] += x2 * x1;
            m[5] += x2 * x2;
        }
        for (int i = 0; i < 6; i++) {
            corr[frame][i] = m[i];
        }
    }
}

// Calculate the square error, given an autocorrelation matrix and predictor
// coefficients.
static float vadpcm_eval(const float corr[restrict static 6],
                         const float coeff[restrict static 2]) {
    return corr[0] +                               //
           corr[2] * coeff[0] * coeff[0] +         //
           corr[5] * coeff[1] * coeff[1] +         //
           2.0f * (corr[4] * coeff[0] * coeff[1] - //
                   corr[1] * coeff[0] -            //
                   corr[3] * coeff[1]);
}

// Calculate the predictor coefficients, given an autocorrelation matrix. The
// coefficients are chosen to minimize vadpcm_eval.
static void vadpcm_solve(const double corr[restrict static 6],
                         double coeff[restrict static 2]) {
    // For the autocorrelation matrix A, we want vector v which minimizes the
    // residual \epsilon,
    //
    // \epsilon = [1|v]^T A [1|v]
    //
    // We can rewrite this as:
    //
    // \epsilon = B + 2 C v + v^T D v
    //
    // Where B, C, and D are submatrixes of A. The minimum value, v, satisfies:
    //
    // D v + C = 0.

    double rel_epsilon = 1.0 / 4096.0;
    coeff[0] = 0.0;
    coeff[1] = 0.0;

    // The element with maximum absolute value is on the diagonal, by the
    // Cauchy-Schwarz inequality.
    double max = corr[0];
    if (corr[2] > max) {
        max = corr[2];
    }
    if (corr[5] > max) {
        max = corr[5];
    }
    double epsilon = max * rel_epsilon;

    // Solve using Gaussian elimination.
    //
    // [a b | x]
    // [b c | y]
    double a = corr[2];
    double b = corr[4];
    double c = corr[5];
    double x = corr[1];
    double y = corr[3];

    // Partial pivoting. Note that a, c are non-negative.
    int pivot = c > a;
    if (pivot) {
        double t;
        t = a;
        a = c;
        c = t;
        t = x;
        x = y;
        y = t;
    }

    // Multiply first row by 1/a: [1 b/a | x/a]
    if (a <= epsilon) {
        // Matrix is close to zero. Just use zero for the predictor
        // coefficients.
        return;
    }
    double a1 = 1.0 / a;
    double b1 = b * a1;
    double x1 = x * a1;

    // Subtract first row * b from second row: [0 c-b1*b | y - x1*b]
    double c2 = c - b1 * b;
    double y2 = y - x1 * b;

    // Multiply second row by 1/c. [0 1 | y2/c2]
    if (fabs(c2) <= epsilon) {
        // Matrix is poorly conditioned or singular. Solve as a first-order
        // system.
        coeff[pivot] = x1;
        return;
    }
    double y3 = y2 / c2;

    // Backsubstitute.
    double x4 = x1 - y3 * b1;

    coeff[pivot] = x4;
    coeff[!pivot] = y3;
}

// Calculate the worst-case error from a frame, given its solved coefficients.
static double vadpcm_eval_solved(const double corr[restrict static 6],
                                 const double coeff[restrict static 2]) {
    // Equivalent to vadpcm_eval(), for the case where coeff are optimal for
    // this autocorrelation matrix.
    //
    // matrix = [k B^T]
    //          [B A  ]
    //
    // solve(A, B) = A^-1 B
    // eval(k, A, B, x) = k - 2B^T x + x^T A x
    // eval(k, A, B, solve(A, B))
    //   = eval(k, A, B, A^-1 B)
    //   = k - 2B^T (A^-1 B) + (A^-1 B)^T A (A^-1 B)
    //   = k - 2B^T A^-1 B + B^T A^-1^T A A^-1 B
    //   = k - 2B^T A^-1 B + B^T A^-1^T B
    //   = k - B^T A^-1 B
    //   = k - B^T solve(A, B)
    return corr[0] - corr[1] * coeff[0] - corr[3] * coeff[1];
}

static int vadpcm_getshift(int min, int max) {
    int shift = 0;
    while (shift < 12 && (min < -8 || 7 < max)) {
        min >>= 1;
        max >>= 1;
        shift++;
    }
    return shift;
}

// Encode audio as VADPCM, given the assignment of each frame to a predictor.
static void vadpcm_encode_data(
    size_t frame_count, void *restrict dest, const int16_t *restrict src,
    const uint8_t *restrict predictors,
    const struct vadpcm_codebook *restrict codebook) {
    uint8_t *destptr = dest;
    int state[4];
    state[0] = 0;
    state[1] = 0;
    for (size_t frame = 0; frame < frame_count; frame++) {
        unsigned predictor = predictors[frame];
        const struct vadpcm_vector *restrict pvec =
            codebook->data + 2 * predictor;
        int accumulator[8], s0, s1, s, a, r, min, max;

        // Calculate the residual with full precision, and figure out the
        // scaling factor necessary to encode it.
        state[2] = src[frame * 16 + 6];
        state[3] = src[frame * 16 + 7];
        min = 0;
        max = 0;
        for (int vector = 0; vector < 2; vector++) {
            s0 = state[vector * 2];
            s1 = state[vector * 2 + 1];
            for (int i = 0; i < 8; i++) {
                accumulator[i] = (src[frame * 16 + vector * 8 + i] << 11) -
                                 s0 * pvec[0].v[i] - s1 * pvec[1].v[i];
            }
            for (int i = 0; i < 8; i++) {
                s = accumulator[i] >> 11;
                if (s < min) {
                    min = s;
                }
                if (s > max) {
                    max = s;
                }
                for (int j = 0; j < 7 - i; j++) {
                    accumulator[i + 1 + j] -= s * pvec[1].v[j];
                }
            }
        }
        int shift = vadpcm_getshift(min, max);

        // Try a range of 3 shift values, and use the shift value that produces
        // the lowest error.
        double best_error = 0.0;
        int min_shift = shift > 0 ? shift - 1 : 0;
        int max_shift = shift < 12 ? shift + 1 : 12;
        for (shift = min_shift; shift <= max_shift; shift++) {
            uint8_t fout[8];
            double error = 0.0;
            s0 = state[0];
            s1 = state[1];
            // int bias = shift > 0 ? 1 << (shift - 1) : 0;
            for (int vector = 0; vector < 2; vector++) {
                for (int i = 0; i < 8; i++) {
                    accumulator[i] = s0 * pvec[0].v[i] + s1 * pvec[1].v[i];
                }
                for (int i = 0; i < 8; i++) {
                    s = src[frame * 16 + vector * 8 + i];
                    a = accumulator[i] >> 11;
                    // Calculate the residual, encode as 4 bits.
                    r = (s - a) >> shift;
                    if (r > 7) {
                        r = 7;
                    } else if (r < -8) {
                        r = -8;
                    }
                    accumulator[i] = r;
                    // Update state to match decoder.
                    int sout = r << shift;
                    for (int j = 0; j < 7 - i; j++) {
                        accumulator[i + 1 + j] += sout * pvec[1].v[j];
                    }
                    sout += a;
                    s0 = s1;
                    s1 = sout;
                    // Track encoding error.
                    double serror = s - sout;
                    error += serror * serror;
                }
                for (int i = 0; i < 4; i++) {
                    fout[vector * 4 + i] = ((accumulator[2 * i] & 15) << 4) |
                                           (accumulator[2 * i + 1] & 15);
                }
            }
            if (shift == min_shift || error < best_error) {
                destptr[frame * 9] = (shift << 4) | predictor;
                memcpy(destptr + frame * 9 + 1, fout, 8);
                state[2] = s0;
                state[3] = s1;
                best_error = error;
            }
        }
        state[0] = state[2];
        state[1] = state[3];
    }
}

#if TEST
#include "lib/vadpcm/test.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#pragma GCC diagnostic ignored "-Wdouble-promotion"

static bool did_fail;

static void test_autocorr(void) {
    // Check that the error for a set of coefficients can be calculated using
    // the autocorrelation matrix.
    static const float coeff[2] = {0.5f, 0.25f};

    // Computationally Easy, Spectrally Good Multipliers for Congruential
    // Pseudorandom Number Generators, Steele and Vigna, Table 7, p.18
    const uint32_t a = 0xd9f5;
    const uint32_t c = 0x6487ed51; // pi << 29.
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
                state = state * a + c;
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
        did_fail = true;
    }
}

static void test_solve(void) {
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
        did_fail = true;
    }
}

static void print_verr(const char *name, const char *func, vadpcm_error err) {
    const char *msg = vadpcm_error_name(err);
    if (msg == NULL) {
        msg = "unknown error";
    }
    fprintf(stderr, "error: %s %s: %s\n", name, func, msg);
}

static int vadpcm_ext4(int x) {
    return x > 7 ? x - 16 : x;
}

static void show_raw(const char *name, const uint8_t *data) {
    fprintf(stderr, "%s: scale = %d, predictor = %d\n%s:", name, data[0] >> 4,
            data[0] & 15, name);
    for (int i = 1; i < kVADPCMFrameByteSize; i++) {
        fprintf(stderr, "%8d%8d", vadpcm_ext4(data[i] >> 4),
                vadpcm_ext4(data[i] & 15));
    }
    fputc('\n', stderr);
}

static void test_reencode(const char *name) {
    // Take a VADPCM file. Decode it, then reencode it with the same codebook,
    // and then decode it again. The decoded PCM data should match.
    struct aiff adpcm;
    struct vadpcm_codebook *b = NULL;
    int16_t *pcm1 = NULL, *pcm2 = NULL;
    uint8_t *adpcm2 = NULL;
    uint8_t *predictors = NULL;

    read_aiff_vadpcm(&adpcm, name);
    vadpcm_error err =
        vadpcm_read_codebook_aifc(&b, adpcm.codebook, adpcm.codebook_size);
    if (err != 0) {
        print_verr(name, "read_codebook", err);
        did_fail = true;
        goto done;
    }
    size_t frame_count = adpcm.audio_size / kVADPCMFrameByteSize;
    if (frame_count == 0) {
        fprintf(stderr, "error: reencode %s: no frames\n", name);
        did_fail = true;
        goto done;
    }
    pcm1 = xmalloc(kVADPCMFrameSampleCount * sizeof(int16_t) * frame_count);
    struct vadpcm_state state;
    memset(&state, 0, sizeof(state));
    err = vadpcm_decode(b, &state, frame_count, pcm1, adpcm.audio);
    if (err != 0) {
        print_verr(name, "decode (first)", err);
        did_fail = true;
        goto done;
    }
    predictors = xmalloc(frame_count);
    for (size_t frame = 0; frame < frame_count; frame++) {
        predictors[frame] =
            ((const uint8_t *)adpcm.audio)[kVADPCMFrameByteSize * frame] & 15;
    }
    adpcm2 = xmalloc(kVADPCMFrameByteSize * frame_count);
    vadpcm_encode_data(frame_count, adpcm2, pcm1, predictors, b);
    pcm2 = xmalloc(kVADPCMFrameSampleCount * sizeof(int16_t) * frame_count);
    memset(&state, 0, sizeof(state));
    err = vadpcm_decode(b, &state, frame_count, pcm2, adpcm2);
    if (err != 0) {
        print_verr(name, "decode (second)", err);
        did_fail = true;
        goto done;
    }
    for (size_t i = 0; i < frame_count * kVADPCMFrameSampleCount; i++) {
        if (pcm1[i] != pcm2[i]) {
            fprintf(stderr,
                    "error: reencode %s: output does not match, "
                    "index = %zu\n",
                    name, i);
            size_t frame = i / kVADPCMFrameSampleCount;
            fputs("vadpcm:\n", stderr);
            show_raw("raw", (const uint8_t *)adpcm.audio +
                                kVADPCMFrameByteSize * frame);
            show_raw("out", adpcm2 + kVADPCMFrameByteSize * frame);
            fputs("pcm:\n", stderr);
            show_pcm_diff(pcm1 + frame * kVADPCMFrameSampleCount,
                          pcm2 + frame * kVADPCMFrameSampleCount);
            did_fail = true;
            goto done;
        }
    }

done:
    free(adpcm.data.data);
    free(b);
    free(pcm1);
    free(pcm2);
    free(adpcm2);
    free(predictors);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    test_autocorr();
    test_solve();
    for (int i = 0; kAIFFNames[i] != NULL; i++) {
        test_reencode(kAIFFNames[i]);
    }
    return did_fail ? 1 : 0;
}

#endif // TEST
