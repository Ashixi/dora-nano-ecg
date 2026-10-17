# Analytical Hardware & Energy Feasibility Profile

**Target Architecture:** ARM Cortex-M4F / Cortex-M33 (e.g., Nordic nRF52840, STM32F401, Ambiq Apollo4)  
**Status:** Analytical Complexity & Microarchitectural Model  
**Author:** Andrii Shumko (Prague, Czech Republic)

---

## 1. Scope & Engineering Disclosure

This document outlines the computational complexity and theoretical power consumption of the DORA ECG inference kernel. 

> **Important Disclosure:**  
> The algorithmic operation count (**18,280 FLOPs/beat**) and host latency (**3.47 µs**) are directly measured from executable code. The cycle counts and energy estimations presented below are derived from an **analytical microarchitectural model** based on official ARM Cortex-M4 technical reference manuals. Empirical physical bench measurements using a DC power analyzer and oscilloscope on custom PCB silicon will be published in Phase 2.

---

## 2. Algorithmic Complexity Breakdown

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

## 3. Microcontroller Cycle Execution Model (Cortex-M4F)

ARM Cortex-M4F features a single-precision floating point unit (FPU) supporting the `VMLA.F32` (Vector Multiply and Accumulate) instruction, executing in 1–2 clock cycles.

Assuming a standard pipelined execution factor of **1.75 cycles per FLOP** (accounting for operand load/store instructions and pipeline register scheduling):

$$\text{Estimated Cycles} = 18,280 \times 1.75 \approx 31,990 \text{ cycles} \approx 32,000 \text{ cycles}$$

At a clock frequency of **64 MHz**:

$$\text{Active Execution Time} = \frac{32,000 \text{ cycles}}{64,000,000 \text{ Hz}} = 5.0 \times 10^{-4} \text{ s} = \mathbf{0.50 \text{ ms}}$$

---

## 4. Theoretical Energy & Duty Cycle Model

At a typical resting heart rate of 75 beats per minute (BPM), the cardiac cycle interval is approximately **800 ms**:

$$\text{CPU Active Duty Cycle} = \frac{0.50 \text{ ms}}{800 \text{ ms}} = 0.0625\%$$

$$\text{CPU Sleep Duty Cycle} = 100\% - 0.0625\% = \mathbf{99.9375\%} \approx \mathbf{99.94\%}$$

### CPU Core Energy Budget (Theoretical)
Under typical Cortex-M4 active power consumption specifications (~3.3V, ~6.4 mA active @ 64 MHz, and ~1.5 µA deep sleep):

1. **Active Core Energy:**  
   $$E_{\text{active}} = 3.3 \text{ V} \times 6.4 \text{ mA} \times 0.50 \text{ ms} = 10.56 \text{ µJ}$$

2. **Sleep Period Energy (799.5 ms):**  
   $$E_{\text{sleep}} = 3.3 \text{ V} \times 1.5 \text{ µA} \times 799.5 \text{ ms} = 3.95 \text{ µJ}$$

3. **Total Theoretical Energy per Beat:**  
   $$E_{\text{beat}} = 10.56 \text{ µJ} + 3.95 \text{ µJ} \approx \mathbf{14.5 \text{ µJ}}$$

This theoretical CPU-only energy budget demonstrates that the mathematical kernel itself imposes practically negligible battery drain.

---

## 5. Practical Firmware Integration Recommendations

When porting `dora_nano` to bare-metal microcontrollers (e.g., STM32, Nordic nRF52, ESP32-S3):

1. **Flash Wait States vs. SRAM Placement:**  
   At clock frequencies above 24–30 MHz, microcontrollers typically introduce 1–3 wait states for internal Flash memory access unless an instruction cache (e.g., ST ART Accelerator) is enabled.  
   *Recommendation:* Place the hot inner inference loops into internal SRAM using linker directives (e.g., `#[link_section = ".ramfunc"]` in Rust or `__attribute__((section(".ramfunc")))` in C) to guarantee 1-cycle zero-wait instruction fetches.

2. **Zero Dynamic Allocation (IEC 62304):**  
   `dora_nano` strictly uses caller-provided buffers via `dora_nano_ecg_init(storage)`. Pre-allocate all storage statically or on the stack to guarantee zero heap fragmentation and deterministic latency.

---

## 6. Phase 2 Hardware Roadmap

In Phase 2, empirical bench measurements will evaluate:
* Full-system active and quiescent power including Analog Front-End (AFE) ADCs (e.g., TI ADS1292R, AD8232, MAX30003).
* BLE radio advertising and telemetry packet transmission overhead.
* LDO regulator quiescent current and battery discharge curves.
