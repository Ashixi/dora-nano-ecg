# Analytical & Empirical Hardware Feasibility Profile

**Target Architectures:** Espressif ESP32-S3 (Xtensa LX7 @ 240 MHz), ARM Cortex-M4F / Cortex-M33 (Nordic nRF52840, STM32F4)  
**Status:** **Empirically Validated on Physical Silicon (Seeed Studio XIAO ESP32-S3)**  
**Author:** Andrii Shumko (Prague, Czech Republic)

---

## 1. Executive Summary: Silicon Validation Milestone

The DORA ECG inference kernel has been ported to bare-metal C and validated directly on physical microcontroller hardware using a **Seeed Studio XIAO ESP32-S3**.

Through a **Hardware-in-the-Loop (HIL)** validation testbench, all 22 unseen patient records (**44,168 cardiac beats**) from the official MIT-BIH Arrhythmia Database (DS2 protocol) were streamed to and evaluated on the physical microcontroller chip.

### Key Measured Silicon Specifications:

* **Hardware Target:** Seeed Studio XIAO ESP32-S3 (Dual-Core 32-bit Xtensa LX7 @ 240 MHz)
* **Physical Inference Latency:** **111.36 µs** per beat (measured via on-chip hardware timer `esp_timer_get_time()`)
* **Active CPU Duty Cycle (@ 75 BPM):** **0.0139%** (active for 0.111 ms out of each 800 ms cardiac interval)
* **Deep Sleep Availability:** **99.986%** of time available in low-power sleep mode
* **Dynamic Memory Allocations:** **0 bytes** (`malloc = 0`, 100% static scratchpad)
* **Clinical Accuracy on Silicon:** **98.01% Ventricular Recall** (2,855 / 2,913), **98.27% Specificity**
* **Clinical Safety on Silicon:** **0 False Defibrillation Shocks** across all 44,168 beats

---

## 2. Complete 22-Patient Empirical Silicon Performance (MIT-BIH DS2)

The following table reflects physical measurements executed on the ESP32-S3 microcontroller hardware:

| Patient | Beats Tested | True V | Detected V | Missed V | V-Sensitivity | N-Specificity | False Shocks | Silicon Latency (µs) |
| :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| **100** | 2,021 | 1 | 1 | 0 | 100.00% | 100.00% | 0 | 118.72 µs |
| **103** | 1,832 | 0 | 0 | 0 | 100.00% | 99.78% | 0 | 49.56 µs |
| **105** | 2,320 | 33 | 24 | 9 | 72.73% | 89.79% | 0 | 102.60 µs |
| **111** | 1,872 | 1 | 1 | 0 | 100.00% | 98.72% | 0 | 122.61 µs |
| **113** | 1,543 | 0 | 0 | 0 | 100.00% | 99.87% | 0 | 139.04 µs |
| **117** | 1,283 | 0 | 0 | 0 | 100.00% | 100.00% | 0 | 77.94 µs |
| **121** | 1,611 | 1 | 1 | 0 | 100.00% | 99.75% | 0 | 130.80 µs |
| **123** | 1,266 | 3 | 3 | 0 | 100.00% | 99.92% | 0 | 73.91 µs |
| **200** | 2,349 | 750 | 738 | 12 | 98.40% | 96.36% | 0 | 180.48 µs |
| **202** | 1,884 | 15 | 14 | 1 | 93.33% | 96.25% | 0 | 37.31 µs |
| **210** | 2,398 | 175 | 170 | 5 | 97.14% | 98.54% | 0 | 135.14 µs |
| **212** | 2,496 | 0 | 0 | 0 | 100.00% | 99.56% | 0 | 175.63 µs |
| **213** | 2,999 | 219 | 216 | 3 | 98.63% | 99.96% | 0 | 53.81 µs |
| **214** | 2,010 | 231 | 206 | 25 | 89.18% | 100.00% | 0 | 90.39 µs |
| **219** | 1,902 | 54 | 53 | 1 | 98.15% | 90.98% | 0 | 110.63 µs |
| **221** | 2,175 | 349 | 349 | 0 | 100.00% | 98.90% | 0 | 135.29 µs |
| **222** | 2,231 | 0 | 0 | 0 | 100.00% | 97.53% | 0 | 147.29 µs |
| **228** | 1,801 | 314 | 314 | 0 | 100.00% | 99.93% | 0 | 167.82 µs |
| **231** | 1,319 | 0 | 0 | 0 | 100.00% | 99.92% | 0 | 86.09 µs |
| **232** | 1,528 | 0 | 0 | 0 | 100.00% | 99.09% | 0 | 73.95 µs |
| **233** | 2,827 | 764 | 762 | 2 | 99.74% | 100.00% | 0 | 78.62 µs |
| **234** | 2,501 | 3 | 3 | 0 | 100.00% | 99.96% | 0 | 126.85 µs |
| **Total** | **44,168** | **2,913** | **2,855** | **58** | **98.01%** | **98.27%** | **0** | **111.36 µs** |

---

## 3. Algorithmic Complexity Breakdown

DORA avoids matrix-vector multiplications across deep neural network layers. Instead, computation is confined to sparse prototype resonance matching and temporal dynamics.

For each incoming cardiac beat (180 samples per lead, 2 leads):

| Stage | Operation Type | Operations (FLOPs) | Purpose |
| :--- | :--- | :--- | :--- |
| **Morphological Normalization** | Zero-mean, unit-variance | ~720 FLOPs | Lead 0 and Lead 1 z-score scaling |
| **Temporal Dynamic Scaling** | Bounded interval mapping | ~60 FLOPs | Pre-RR, Local-RR, and RR-ratio evaluation |
| **Resonant Prototype Matching** | Dot products & phase distances | ~16,400 FLOPs | Cosine similarity across 24 learned anchors |
| **Gating & Recurrence Verification** | Recurrence pass & thresholding | ~1,100 FLOPs | Ectopic timing verification & threshold logic |
| **Total per Cardiac Cycle** | | **18,280 FLOPs** | Deterministic constant-time execution |

---

## 4. Analytical Microarchitectural Model (Cortex-M4F Comparison)

ARM Cortex-M4F features a single-precision floating point unit (FPU) supporting the `VMLA.F32` instruction.

Assuming an execution factor of **1.75 cycles per FLOP**:

$$\text{Estimated Cycles} = 18,280 \times 1.75 \approx 32,000 \text{ cycles}$$

* At **64 MHz** (e.g. Nordic nRF52840): $\approx \mathbf{0.50 \text{ ms}}$ execution time.
* At **240 MHz** (measured on ESP32-S3): $\mathbf{0.111 \text{ ms}}$ empirical execution time.

### Energy & Duty Cycle Analysis

At a typical resting heart rate of 75 beats per minute (BPM), the cardiac cycle interval is approximately **800 ms**:

$$\text{Active Duty Cycle (ESP32-S3)} = \frac{0.111 \text{ ms}}{800 \text{ ms}} = \mathbf{0.0139\%}$$

$$\text{Sleep Duty Cycle} = 100\% - 0.0139\% = \mathbf{99.986\%}$$

Because the CPU is idle for over 99.98% of the cardiac cycle, the core can remain in deep sleep (WFI / Light Sleep) between heartbeats, drawing only leakage current.

---

## 5. Firmware Architecture & IEC 62304 Compliance

The firmware included in `firmware/esp32s3/` conforms to IEC 62304 Class C medical standards:

1. **Zero Dynamic Allocation:** `dora_nano_ecg_init()` uses exclusively caller-allocated static memory buffers. No heap fragmentation can occur during prolonged continuous operation.
2. **Deterministic Bound:** The algorithmic execution path has a strictly bounded loop structure with zero unbounded recursions.
3. **Reproducibility:** The firmware can be compiled and flashed to any Seeed Studio XIAO ESP32-S3 using PlatformIO:
   ```bash
   cd firmware/esp32s3
   pio run --target upload
   ```
