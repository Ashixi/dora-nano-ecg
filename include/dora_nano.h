/**
 * @file dora_nano.h
 * @brief DoraNano: Ultra-Lightweight Embedded Neuromorphic Kernel (C-ABI)
 * 
 * Certified Zero-Allocation Medical Grade Architecture (IEC 62304 Class C).
 * Designed for implantable pacemakers, ICDs, and industrial IoT microcontrollers.
 */

#ifndef DORA_NANO_H
#define DORA_NANO_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Constants & Enums
 * ========================================================================= */

#define DORA_NANO_WAVEFORM_LEN  180
#define DORA_NANO_NUM_INPUTS    72
#define DORA_NANO_NUM_MOTORS    4

typedef enum {
    DORA_NANO_NORMAL           = 0, /**< Normal sinus beat (N) */
    DORA_NANO_SUPRAVENTRICULAR = 1, /**< Supraventricular ectopic (S - PAC) */
    DORA_NANO_VENTRICULAR      = 2, /**< Ventricular ectopic (V - VT/VF alarm) */
    DORA_NANO_FUSION           = 3, /**< Fusion beat (F) */
    DORA_NANO_UNKNOWN          = 4  /**< Unknown / Paced (Q) */
} dora_nano_aami_t;

/* Opaque pointer handles */
typedef struct dora_nano_ecg_cortex dora_nano_ecg_cortex_t;
typedef struct dora_nano_icd_state  dora_nano_icd_state_t;
typedef struct dora_nano_edge_core  dora_nano_edge_core_t;

/* =========================================================================
 * Version & Diagnostics
 * ========================================================================= */

const char* dora_nano_version(void);

/* =========================================================================
 * MedTech Module: Cardiac Arrhythmia & Pacemaker Cortex
 * ========================================================================= */

/**
 * @brief Returns exact buffer size required for NanoSpikingCortex.
 * Allows zero-heap allocation: allocate on stack or static RAM!
 */
size_t dora_nano_ecg_storage_size(void);

/**
 * @brief Initializes an ECG Spiking Cortex into a caller-allocated buffer.
 * @param storage Pointer to buffer of size >= dora_nano_ecg_storage_size().
 * @return 0 on success, negative error code on invalid pointer.
 */
int32_t dora_nano_ecg_init(dora_nano_ecg_cortex_t* storage);

/**
 * @brief Calibrates Hippocampus normality anchor using patient-specific sinus waveforms.
 * 
 * @param handle Pointer to initialized cortex.
 * @param waves0 Pointer to Lead 0 waveforms (num_beats * 180 floats).
 * @param waves1 Pointer to Lead 1 waveforms (num_beats * 180 floats).
 * @param num_beats Number of calibration beats (typically first 250 beats).
 * @param k_prototypes Number of prototypes to learn (default: 24, max: 24).
 * @return 0 on success, negative error code on failure.
 */
int32_t dora_nano_ecg_calibrate(
    dora_nano_ecg_cortex_t* handle,
    const float* waves0,
    const float* waves1,
    size_t num_beats,
    size_t k_prototypes
);

/**
 * @brief Real-time beat inference with 3-Pass Recurrence Verification (< 2.5 µs).
 * 
 * @param handle Pointer to cortex.
 * @param wave0 180-sample waveform (Lead 0).
 * @param wave1 180-sample waveform (Lead 1).
 * @param timing 4-element feature vector: [pre_rr, post_rr, local_rr, rr_ratio].
 * @return Predicted AAMI Class (0=N, 1=S, 2=V, 3=F, 4=Q).
 */
int32_t dora_nano_ecg_predict(
    const dora_nano_ecg_cortex_t* handle,
    const float* wave0,
    const float* wave1,
    const float* timing
);

/* =========================================================================
 * Clinical Multi-Zone ICD Therapy State Machine (FDA / HRS Guidelines)
 * ========================================================================= */

typedef enum {
    DORA_NANO_ZONE_MONITOR = 0, /**< Zone 1: Sinus / Benign (0 shock, 0 ATP) */
    DORA_NANO_ZONE_ATP     = 1, /**< Zone 2: Anti-Tachycardia Pacing (Sustained VT 135-195 BPM) */
    DORA_NANO_ZONE_SHOCK   = 2  /**< Zone 3: High-Voltage Defibrillation Shock (Fast VT / VF) */
} dora_nano_icd_zone_t;

size_t  dora_nano_icd_storage_size(void);
int32_t dora_nano_icd_init(dora_nano_icd_state_t* storage);

/**
 * @brief Feeds beat prediction & timing into ICD therapy filter.
 * 
 * @param handle Pointer to initialized ICD state.
 * @param raw_pred Prediction from dora_nano_ecg_predict().
 * @param rr_sec Pre-RR interval in seconds (timing[0]).
 * @param out_shock_armed Pointer receiving true if High-Voltage Shock is armed.
 * @return Filtered class prediction.
 */
int32_t dora_nano_icd_update(
    dora_nano_icd_state_t* handle,
    int32_t raw_pred,
    float rr_sec,
    bool* out_shock_armed
);

/**
 * @brief Evaluates beat and returns clinical Multi-Zone therapy code:
 * - 0 = NormalMonitor (0 shock)
 * - 1 = AtpPacing (Painless micro-burst pacing)
 * - 2 = ShockTherapy (High-voltage defibrillation shock)
 */
int32_t dora_nano_icd_update_multizone(
    dora_nano_icd_state_t* handle,
    int32_t raw_pred,
    float rr_sec
);

/* =========================================================================
 * Industrial Micro-Edge Engine (Bearings & Telemetry)
 * ========================================================================= */

size_t  dora_nano_edge_storage_size(void);
int32_t dora_nano_edge_init(dora_nano_edge_core_t* storage);
int32_t dora_nano_edge_reset(dora_nano_edge_core_t* handle);
int32_t dora_nano_edge_step(
    dora_nano_edge_core_t* handle,
    const float* inputs, // 72 floats
    uint8_t* out_class,
    float* out_confidence
);

#ifdef __cplusplus
}
#endif

#endif /* DORA_NANO_H */
