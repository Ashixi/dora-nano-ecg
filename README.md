# DORA: Dual Ontogenic Resonance Architecture

**DORA** is a novel brain-inspired neural network architecture.

It is neither a conventional deep neural network reliant on brute-force matrix multiplications, nor an attempt to simulate biological wetware on analog silicon. It is an independent mathematical framework that abstracts fundamental principles of biological information processing.

---

### What is DORA?

* **D — Dual:**  
  The architecture operates across two complementary levels of processing (analogous to the neocortex–hippocampus dynamic): continuous baseline tracking coupled with a spiking resonance memory layer.

* **O — Ontogenic:**  
  In biology, *ontogenesis* is the development of an organism from a single cell. A DORA model is not a static, pre-configured grid of layers: it **grows literally from scratch, starting from a single soma**, autonomously developing its own topology, internal structure, and pathways.

* **R — Resonance:**  
  Instead of dense linear algebra and deep feedforward matrices, computation is governed by **temporal and phase resonance** within a bounded state space. The system naturally activates when incoming signal dynamics fall into resonance with internalized patterns.

* **A — Architecture:**  
  Do you really want me to explain what "architecture" means? [Fine, here's Wikipedia](https://en.wikipedia.org/wiki/Software_architecture). In our context: a unified neural framework designed for continuous, dynamic temporal signals.

---

### Application: Real-Time ECG Analysis

This repository demonstrates the application of the DORA architecture to real-time electrocardiogram (ECG) analysis.

Due to its foundational principles, the resulting engine does not rely on artificial pruning, distillation, or aggressive quantization hacks to fit small devices. It is **inherently compact (< 20 KB)** by nature. It evaluates each cardiac cycle in microseconds with minimal energy footprint, enabling years of autonomous operation on standard microcontrollers powered by a single coin-cell battery.

---

### Key Technical Specifications

DORA achieves high-precision inference on ultra-constrained edge silicon without relying on cloud connectivity, deep neural accelerators, or floating-point co-processors:

| Specification | Measured Value | Practical Context / Hardware Target |
| :--- | :--- | :--- |
| **Binary Footprint** | **18.5 KB** | Zero dynamic heap allocations (`#![no_std]` bare-metal compatible) |
| **Inference Latency** | **2.62 µs** / beat | Evaluated on standard x86_64 @ 3.4 GHz |
| **MCU Execution Timing** | **~500 µs** (32,000 cycles) | ARM Cortex-M4 @ 64 MHz with hardware FPU |
| **Algorithmic Complexity** | **18,280 FLOPs** / beat | Deterministic execution time, zero jitter |
| **Energy Consumption** | **10.62 µJ** / cardiac cycle | Enables **99.94% Deep Sleep / WFI** duty cycle |
| **Theoretical Battery Life** | **9.0+ years** | Continuous 24/7 monitoring on a single 350 mAh coin cell |

---

### Clinical Performance on Benchmark (MIT-BIH DS2)

Evaluated under the rigorous inter-patient testing standard (DS2 unseen test cohort: **22 patients, 44,168 total cardiac beats**):

* **Ventricular Ectopic Sensitivity (Class V Recall):** **98.01%** (2,855 of 2,913 events detected)
* **Normal Sinus Rhythm Specificity (Class N):** **98.61%**
* **Defibrillation False Shocks on Normal Heart:** **0 / 39,234 beats (0.0000%)**
* **Normal Heart ICD Safety Score:** **100.0000%**

#### Zero Data Leakage & Strict Causality Protocol
* **Strict Inter-Patient Separation:** $\text{DS1}_{\text{train}} \cap \text{DS2}_{\text{test}} = \emptyset$. Zero patient overlap.
* **Strict Temporal Causality:** Initial 250 beats used exclusively for personal baseline anchor acquisition; all evaluation strictly conducted on subsequent unseen signals ($t \ge 250$).
* **Zero Future Lookahead:** No reliance on post-RR intervals or non-causal smoothing filters. Each beat is evaluated strictly upon arrival.
* **Zero Hardcoded Constants:** No patient-specific thresholds or signal prototypes are baked into the compiled binary.

