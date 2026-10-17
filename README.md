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

DORA achieves high-precision inference on ultra-constrained edge silicon without relying on cloud connectivity, heavy tensor accelerators, or external co-processors:

| Specification | Value | Status / Hardware Context |
| :--- | :--- | :--- |
| **Binary Footprint** | **18.5 KB** | Measured (MSVC/LLVM `.text` + `.rdata`, zero dynamic allocations) |
| **Inference Latency** | **3.47 µs** / beat | Measured on host x86_64 @ 3.4 GHz |
| **Algorithmic Complexity** | **18,280 FLOPs** / beat | Deterministic operation count (exact additions & multiplications) |
| **MCU Execution Timing** | **~500 µs** (~32,000 cycles) | Analytical model: ARM Cortex-M4 @ 64 MHz with FPU |
| **CPU Core Energy** | **~10.62 µJ** / cardiac cycle | Analytical model: enables **99.94% Deep Sleep / WFI** duty cycle |
| **Theoretical CPU Autonomy** | **9.0+ years** | Theoretical compute budget on a 350 mAh coin cell (CPU only) |

> **Engineering Transparency Note:** Binary footprint, algorithmic FLOP count, and host latency are directly measured. The Cortex-M4 cycle count and energy figures are derived from an analytical architectural model based on ARM instruction execution timings. Full empirical hardware profiling (including ADC analog frontend and telemetry overhead) will be conducted on physical silicon boards in Phase 2. See [docs/HARDWARE_PROFILE.md](docs/HARDWARE_PROFILE.md) for full derivations.

---

### Technical Documentation & Architecture Deep-Dives

* 🔬 **[Hardware & Energy Feasibility Profile](docs/HARDWARE_PROFILE.md):** Detailed microarchitectural cycle modeling on ARM Cortex-M4F, instruction timings (`VMLA.F32`), and theoretical energy budgets.
* 🛡️ **[Clinical Safety & Defibrillation Protocol](docs/CLINICAL_SAFETY.md):** Multi-Zone ICD state machine, ectopic prematurity gating, and the physiological basis for 0 false shocks.
* 💻 **[C-ABI Integration Reference](docs/C_API_REFERENCE.md):** Quickstart guide and code examples for linking `dora_nano.h` into bare-metal C/C++ firmware.


---

### Clinical Performance on Benchmark (MIT-BIH DS2)

Evaluated under the rigorous inter-patient testing standard (DS2 unseen test cohort: **22 patients, 44,168 total cardiac beats**):

* **Ventricular Ectopic Sensitivity (Class V Recall):** **98.01%** (2,855 of 2,913 events detected)
* **Normal Sinus Rhythm Specificity (Class N):** **98.27%** (38,555 of 39,234 normal beats)
* **Defibrillation False Shocks on Normal Heart:** **0 / 39,234 beats (0.0000%)**
* **Normal Heart ICD Safety Score:** **100.0000%**

#### Zero Data Leakage & Strict Causality Protocol
* **Strict Inter-Patient Separation:** $\text{DS1}_{\text{train}} \cap \text{DS2}_{\text{test}} = \emptyset$. Zero patient overlap.
* **Strict Temporal Causality:** Initial 250 beats used exclusively for personal baseline anchor acquisition; all evaluation strictly conducted on subsequent unseen signals ($t \ge 250$).
* **Zero Future Lookahead:** No reliance on post-RR intervals or non-causal smoothing filters. Each beat is evaluated strictly upon arrival.
* **Zero Hardcoded Constants:** No patient-specific thresholds or signal prototypes are baked into the compiled binary.

---

### Benchmark Comparison with Published Literature (MIT-BIH DS2)

The table below compares DORA against established published methods evaluated under the standard inter-patient protocol (de Chazal et al. DS1/DS2 split):

| Methodology & Publication | Architecture / Framework | Model Footprint | Target Hardware | V-Recall (Sensitivity) | Dynamic Heap (`malloc`) |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **de Chazal et al. (2004)** *(IEEE TBME)* | Linear Discriminants (LDC) | ~500 KB (C) | Embedded MCU | **77.7%** | Static |
| **Kiranyaz et al. (2016)** *(IEEE TBME)* | 1D Convolutional Neural Network | ~15–40 MB (PyTorch) | Desktop / GPU | **~90.0%** | Heavy Heap |
| **Mondéjar-Guerra et al. (2019)** *(BSPC)* | Ensemble (SVM + RF + kNN + CNN) | ~120 MB (Python/TF) | Server / PC | **94.7%** | Heavy Heap |
| **Standard TinyML / TFLite (2022–2024)** | Quantized 8-bit CNN / Autoencoders| 80–250 KB (C++) | ARM Cortex-M4 | **~88.5%** | Stack / Heap buffers |
| **DORA Nano ECG — Andrii Shumko (2026)** | **Dual Ontogenic Resonance (DORA)** | **18.5 KB (Pure Rust)** | **Bare-Metal MCU (Cortex-M4)**| **98.01%** | **ZERO (`#![no_std]`)** |

* **Zero Memory Allocation:** DORA is the only deep architecture in this comparison compliant with IEC 62304 Class C requirements (zero heap fragmentation risk for medical implants).
* **ICD Therapy Safety:** DORA couples high ventricular sensitivity (98.01%) with a Multi-Zone state machine, eliminating false shocks on normal sinus rhythm (**0 / 39,234 beats, 100.00% safety**).

---

### Quickstart: Reproducing the Benchmark

You can reproduce the exact benchmark results on your machine using the precompiled zero-dependency kernel.

#### 1. Clone the repository
```bash
git clone https://github.com/Ashixi/dora-nano-ecg.git
cd dora-nano-ecg
```

#### 2. Provide the MIT-BIH Arrhythmia Database
Download the official MIT-BIH Arrhythmia Database records (`.dat` and `.atr` files) from [PhysioNet MIT-BIH](https://physionet.org/content/mitdb/1.0.0/) into a `./mitdb` or `./data/mitdb` directory.

#### 3. Run the benchmark
```bash
cargo run --release
```
Or specify a custom path to your dataset folder:
```bash
cargo run --release -- path/to/mitdb
```

The benchmark runner will evaluate all 22 unseen DS2 patients (**44,168 beats**) in ~400 ms and output the complete patient-by-patient validation table.

---

### Project License & Intellectual Property Notice

* **Benchmark Runner & Data Parsers (`src/`, `build.rs`):** Released under the permissive MIT License.
* **DORA Precompiled Inference Kernel (`lib/`, `include/`):** Provided for non-commercial research, academic benchmarking, and technical evaluation. All underlying somatic equations, resonance topologies, and core learning mechanics remain proprietary intellectual property. For commercial licensing, hardware integration, or clinical pilot inquiries, please contact the author.

---

### Author & Support

Developed by **Andrii Shumko** — Independent Engineer based in Prague, Czech Republic.

* **GitHub:** [@Ashixi](https://github.com/Ashixi)
* **LinkedIn:** [Andrii Shumko](https://www.linkedin.com/in/andrii-shumko)
* **X (Twitter):** [@andrii_shumko](https://x.com/andrii_shumko)

If you find this research valuable or would like to support independent hardware and R&D development:
* ☕ **Buy Me a Coffee:** [buymeacoffee.com/andriishumko](https://buymeacoffee.com/andriishumko)
* 🏦 **Monobank Jar:** [send.monobank.ua/jar/Act5bqpchr](https://send.monobank.ua/jar/Act5bqpchr)

---

<p align="center">🇺🇦 Crafted with dedication by an independent Ukrainian engineer.</p>
