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
