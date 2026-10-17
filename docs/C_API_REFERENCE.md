# C-ABI Integration Reference (`dora_nano.h`)

The DORA ECG kernel exposes a clean, certified C-ABI with **zero dynamic heap allocations** (`malloc`/`free`). All memory is allocated by the caller (either on the stack or in static `.bss` RAM).

---

## 1. Quick Integration Example (Bare-Metal C)

```c
#include "dora_nano.h"
#include <stdio.h>

int main(void) {
    printf("Core Version: %s\n", dora_nano_version());

    // 1. Query required storage sizes
    size_t cortex_bytes = dora_nano_ecg_storage_size();
    size_t icd_bytes    = dora_nano_icd_storage_size();

    // 2. Allocate storage on stack or static RAM (Zero Heap Allocations!)
    uint8_t cortex_storage[cortex_bytes];
    uint8_t icd_storage[icd_bytes];

    // 3. Initialize state machines
    dora_nano_ecg_init((dora_nano_ecg_cortex_t*)cortex_storage);
    dora_nano_icd_init((dora_nano_icd_state_t*)icd_storage);

    // 4. Calibrate normal sinus anchor (first ~250 normal beats)
    // float waves_lead0[250 * 180];
    // float waves_lead1[250 * 180];
    // dora_nano_ecg_calibrate(cortex_storage, waves_lead0, waves_lead1, 250, 24);

    // 5. Real-time per-beat inference
    float current_wave0[180]; // Lead 0 window centered on R-peak
    float current_wave1[180]; // Lead 1 window
    float timing[4] = { 0.82f, 0.81f, 0.82f, 1.0f }; // [pre_rr, post_rr, local_rr, rr_ratio]

    int32_t pred_class = dora_nano_ecg_predict(
        (const dora_nano_ecg_cortex_t*)cortex_storage,
        current_wave0,
        current_wave1,
        timing
    );

    // 6. Clinical Multi-Zone ICD Therapy Update
    int32_t therapy_zone = dora_nano_icd_update_multizone(
        (dora_nano_icd_state_t*)icd_storage,
        pred_class,
        timing[0] // pre-RR interval in seconds
    );

    // therapy_zone:
    // 0 = NormalMonitor (0 shock, 0 pacing)
    // 1 = AtpPacing (Anti-Tachycardia Pacing)
    // 2 = ShockTherapy (High-Voltage Defibrillation)

    return 0;
}
```

---

## 2. API Function Signatures

### Version & Diagnostics
```c
const char* dora_nano_version(void);
```

### Cortex Management
```c
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
```

### Multi-Zone Therapy State Machine
```c
size_t  dora_nano_icd_storage_size(void);
int32_t dora_nano_icd_init(dora_nano_icd_state_t* storage);
int32_t dora_nano_icd_update_multizone(
    dora_nano_icd_state_t* handle,
    int32_t raw_pred,
    float rr_sec
);
```
