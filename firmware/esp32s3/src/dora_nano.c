#include "dora_nano.h"
#include <math.h>
#include <string.h>

const char* dora_nano_version(void) {
    return "DoraNano Embedded Neuromorphic Kernel v2.0 (Zero-Malloc, ESP32-S3 Optimized)";
}

static inline float fast_sqrt(float val) {
    if (val <= 0.0f) return 0.0f;
    return sqrtf(val);
}

/* =========================================================================
 * NanoHippocampus Implementation
 * ========================================================================= */

static void hippo_init(nano_hippocampus_t* hippo) {
    memset(hippo, 0, sizeof(nano_hippocampus_t));
    hippo->count = 0;
}

static bool hippo_add_prototype(nano_hippocampus_t* hippo, const float* wave) {
    if (hippo->count >= DORA_NANO_MAX_PROTOTYPES) {
        return false;
    }

    float norm_sq = 0.0f;
    for (size_t i = 0; i < DORA_NANO_WAVEFORM_LEN; ++i) {
        norm_sq += wave[i] * wave[i];
    }

    float mag = (norm_sq > 1e-12f) ? fast_sqrt(norm_sq) : 1e-6f;
    float* proto = hippo->prototypes[hippo->count];
    for (size_t i = 0; i < DORA_NANO_WAVEFORM_LEN; ++i) {
        proto[i] = wave[i] / mag;
    }

    hippo->count++;
    return true;
}

static void hippo_query(const nano_hippocampus_t* hippo, const float* wave, float* out_sim, float* out_mismatch) {
    if (hippo->count == 0) {
        *out_sim = 1.0f;
        *out_mismatch = 0.0f;
        return;
    }

    float norm_sq = 0.0f;
    for (size_t i = 0; i < DORA_NANO_WAVEFORM_LEN; ++i) {
        norm_sq += wave[i] * wave[i];
    }
    float mag = (norm_sq > 1e-12f) ? fast_sqrt(norm_sq) : 1e-6f;

    float max_res = -1.0f;
    for (size_t p = 0; p < hippo->count; ++p) {
        const float* proto = hippo->prototypes[p];
        float dot = 0.0f;
        for (size_t i = 0; i < DORA_NANO_WAVEFORM_LEN; ++i) {
            dot += wave[i] * proto[i];
        }
        float sim = dot / mag;
        if (sim > max_res) {
            max_res = sim;
        }
    }

    float mismatch = (1.0f - max_res > 0.0f) ? (1.0f - max_res) : 0.0f;
    *out_sim = max_res;
    *out_mismatch = mismatch;
}

static void hippo_train(nano_hippocampus_t* hippo, const float* waves, size_t num_beats, size_t max_k) {
    if (num_beats == 0 || waves == NULL) return;
    hippo->count = 0;
    size_t target_k = (max_k > DORA_NANO_MAX_PROTOTYPES) ? DORA_NANO_MAX_PROTOTYPES : max_k;

    /* 1. First prototype is median normal beat */
    const float* median_beat = &waves[(num_beats / 2) * DORA_NANO_WAVEFORM_LEN];
    hippo_add_prototype(hippo, median_beat);

    /* 2. Iteratively add worst match */
    while (hippo->count < target_k && hippo->count < num_beats) {
        float worst_sim = 1.0f;
        size_t worst_idx = 0;

        for (size_t idx = 0; idx < num_beats; idx += 5) {
            float sim = 0.0f, mismatch = 0.0f;
            hippo_query(hippo, &waves[idx * DORA_NANO_WAVEFORM_LEN], &sim, &mismatch);
            if (sim < worst_sim) {
                worst_sim = sim;
                worst_idx = idx;
            }
        }

        if (worst_sim > 0.985f) {
            break;
        }
        hippo_add_prototype(hippo, &waves[worst_idx * DORA_NANO_WAVEFORM_LEN]);
    }
}

/* =========================================================================
 * NanoSpikingCortex Implementation
 * ========================================================================= */

size_t dora_nano_ecg_storage_size(void) {
    return sizeof(dora_nano_ecg_cortex_t);
}

int32_t dora_nano_ecg_init(dora_nano_ecg_cortex_t* storage) {
    if (!storage) return -1;
    hippo_init(&storage->hippo0);
    hippo_init(&storage->hippo1);
    storage->v_threshold = 1.6f;
    return 0;
}

int32_t dora_nano_ecg_calibrate(
    dora_nano_ecg_cortex_t* handle,
    const float* waves0,
    const float* waves1,
    size_t num_beats,
    size_t k_prototypes
) {
    if (!handle || !waves0 || !waves1 || num_beats == 0) return -1;
    hippo_train(&handle->hippo0, waves0, num_beats, k_prototypes);
    hippo_train(&handle->hippo1, waves1, num_beats, k_prototypes);
    return 0;
}

int32_t dora_nano_ecg_predict(
    const dora_nano_ecg_cortex_t* handle,
    const float* wave0,
    const float* wave1,
    const float* timing
) {
    if (!handle || !wave0 || !wave1 || !timing) return DORA_NANO_UNKNOWN;

    /* PASS 1: Primary Dual-Lead Evaluation */
    float sim0 = 0.0f, mismatch0 = 0.0f;
    float sim1 = 0.0f, mismatch1 = 0.0f;
    hippo_query(&handle->hippo0, wave0, &sim0, &mismatch0);
    hippo_query(&handle->hippo1, wave1, &sim1, &mismatch1);

    float mean_mismatch = (mismatch0 + mismatch1) * 0.5f;
    float dual_mismatch = (mismatch0 < mismatch1) ? mismatch0 : mismatch1;

    float qrs_thresh0 = fabsf(wave0[90]) * 0.25f;
    float qrs_width0 = 0.0f;
    for (size_t i = 65; i < 115; ++i) {
        if (fabsf(wave0[i]) > qrs_thresh0) qrs_width0 += 1.0f;
    }
    float qrs_w0 = qrs_width0 / 50.0f;

    float qrs_thresh1 = fabsf(wave1[90]) * 0.25f;
    float qrs_width1 = 0.0f;
    for (size_t i = 65; i < 115; ++i) {
        if (fabsf(wave1[i]) > qrs_thresh1) qrs_width1 += 1.0f;
    }
    float qrs_w1 = qrs_width1 / 50.0f;
    float max_qrs_w = (qrs_w0 > qrs_w1) ? qrs_w0 : qrs_w1;

    float rr_sec = timing[0];
    float rr_ratio = timing[3];
    float post_rr_ratio = timing[1] / (timing[2] + 1e-6f);

    float i_mismatch = (mean_mismatch > 0.09f) ? (mean_mismatch - 0.09f) * 10.0f : 0.0f;
    float i_dual     = (dual_mismatch > 0.06f) ? (dual_mismatch - 0.06f) * 12.0f : 0.0f;
    float i_width    = (max_qrs_w > 0.60f)     ? (max_qrs_w - 0.60f) * 8.0f : 0.0f;
    float i_pre      = (rr_ratio < 0.88f)      ? (0.88f - rr_ratio) * 6.0f : 0.0f;
    float i_post     = (post_rr_ratio > 1.15f) ? (post_rr_ratio - 1.15f) * 4.0f : 0.0f;

    float soma_v_potential = i_mismatch + i_dual + i_width + i_pre + i_post;
    bool has_normal_lead = (mismatch0 < 0.06f) || (mismatch1 < 0.06f);

    bool is_ectopic_timing = (rr_ratio < 0.94f)
        || (rr_sec < 0.44f)
        || (dual_mismatch > 0.11f && max_qrs_w > 0.58f)
        || (mean_mismatch > 0.16f);

    bool candidate_v = !has_normal_lead
        && (soma_v_potential > handle->v_threshold)
        && is_ectopic_timing
        && (dual_mismatch > 0.08f || (mean_mismatch > 0.12f && max_qrs_w > 0.58f) || (i_pre > 0.0f && max_qrs_w > 0.62f));

    /* PASS 2 & 3: Multi-Pass Recurrent Re-verification */
    if (candidate_v) {
        /* Pass 2: Differential dipole vector energy (V0 - V1) */
        float diff_energy = 0.0f;
        for (size_t i = 70; i < 110; ++i) {
            float diff = wave0[i] - wave1[i];
            diff_energy += diff * diff;
        }

        /* Suppress common-mode artifact */
        if (diff_energy < 0.20f) {
            candidate_v = false;
        }

        /* Pass 3: T-wave discordance confirmation (samples 110..150) */
        float t_area = 0.0f;
        for (size_t i = 110; i < 150; ++i) {
            t_area += fabsf(wave0[i]);
        }
        if (t_area < 2.0f && max_qrs_w < 0.65f) {
            candidate_v = false;
        }
    }

    if (candidate_v) {
        return DORA_NANO_VENTRICULAR;
    }

    /* Supraventricular (Class S) */
    if (rr_ratio < 0.86f && post_rr_ratio > 1.08f && mean_mismatch < 0.08f) {
        return DORA_NANO_SUPRAVENTRICULAR;
    }

    return DORA_NANO_NORMAL;
}

/* =========================================================================
 * NanoIcdStateMachine Implementation
 * ========================================================================= */

size_t dora_nano_icd_storage_size(void) {
    return sizeof(dora_nano_icd_state_t);
}

int32_t dora_nano_icd_init(dora_nano_icd_state_t* storage) {
    if (!storage) return -1;
    storage->v_run = 0;
    return 0;
}

int32_t dora_nano_icd_update(
    dora_nano_icd_state_t* handle,
    int32_t raw_pred,
    float rr_sec,
    bool* out_shock_armed
) {
    if (!handle) return -1;
    int32_t zone = dora_nano_icd_update_multizone(handle, raw_pred, rr_sec);
    if (out_shock_armed) {
        *out_shock_armed = (zone == DORA_NANO_ZONE_SHOCK);
    }
    return raw_pred;
}

int32_t dora_nano_icd_update_multizone(
    dora_nano_icd_state_t* handle,
    int32_t raw_pred,
    float rr_sec
) {
    if (!handle) return -1;
    float bpm = (rr_sec > 0.05f) ? (60.0f / rr_sec) : 75.0f;

    if (raw_pred == DORA_NANO_VENTRICULAR) {
        handle->v_run++;
    } else {
        handle->v_run = 0;
    }

    /* Zone 3: High-Voltage Defibrillation Shock */
    if ((bpm >= 195.0f && handle->v_run >= 3) || (bpm >= 150.0f && handle->v_run >= 6)) {
        return DORA_NANO_ZONE_SHOCK;
    }

    /* Zone 2: Anti-Tachycardia Pacing (ATP) */
    if (bpm >= 135.0f && handle->v_run >= 3) {
        return DORA_NANO_ZONE_ATP;
    }

    /* Zone 1: Inhibit therapy */
    return DORA_NANO_ZONE_MONITOR;
}
