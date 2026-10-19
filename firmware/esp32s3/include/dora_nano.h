#ifndef DORA_NANO_H
#define DORA_NANO_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DORA_NANO_WAVEFORM_LEN   180
#define DORA_NANO_MAX_PROTOTYPES 24

typedef enum {
    DORA_NANO_NORMAL           = 0, /* Normal sinus beat (N) */
    DORA_NANO_SUPRAVENTRICULAR = 1, /* Supraventricular ectopic (S) */
    DORA_NANO_VENTRICULAR      = 2, /* Ventricular ectopic (V - VT/VF alarm) */
    DORA_NANO_FUSION           = 3, /* Fusion beat (F) */
    DORA_NANO_UNKNOWN          = 4  /* Unknown (Q) */
} dora_nano_aami_t;

typedef enum {
    DORA_NANO_ZONE_MONITOR = 0, /* Zone 1: Sinus / Benign */
    DORA_NANO_ZONE_ATP     = 1, /* Zone 2: Anti-Tachycardia Pacing */
    DORA_NANO_ZONE_SHOCK   = 2  /* Zone 3: Defibrillation Shock */
} dora_nano_icd_zone_t;

/* Structures */
typedef struct {
    float prototypes[DORA_NANO_MAX_PROTOTYPES][DORA_NANO_WAVEFORM_LEN];
    size_t count;
} nano_hippocampus_t;

typedef struct dora_nano_ecg_cortex {
    nano_hippocampus_t hippo0;
    nano_hippocampus_t hippo1;
    float v_threshold;
} dora_nano_ecg_cortex_t;

typedef struct dora_nano_icd_state {
    size_t v_run;
} dora_nano_icd_state_t;

/* API */
const char* dora_nano_version(void);

size_t  dora_nano_ecg_storage_size(void);
int32_t dora_nano_ecg_init(dora_nano_ecg_cortex_t* storage);
int32_t dora_nano_ecg_calibrate(
    dora_nano_ecg_cortex_t* handle,
    const float* waves0,
    const float* waves1,
    size_t num_beats,
    size_t k_prototypes
);
int32_t dora_nano_ecg_predict(
    const dora_nano_ecg_cortex_t* handle,
    const float* wave0,
    const float* wave1,
    const float* timing
);

size_t  dora_nano_icd_storage_size(void);
int32_t dora_nano_icd_init(dora_nano_icd_state_t* storage);
int32_t dora_nano_icd_update(
    dora_nano_icd_state_t* handle,
    int32_t raw_pred,
    float rr_sec,
    bool* out_shock_armed
);
int32_t dora_nano_icd_update_multizone(
    dora_nano_icd_state_t* handle,
    int32_t raw_pred,
    float rr_sec
);

#ifdef __cplusplus
}
#endif

#endif /* DORA_NANO_H */
