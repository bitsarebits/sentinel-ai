# Sentinel-AI: Multithreaded Network Intrusion Detection System

> 🚧 **Architectural Focus & Disclaimer**
> This project was built to explore low-level network programming, multithreading, and C-to-AI integration. 
> **Note on the AI Model:** The included PyTorch autoencoder is strictly a "toy model" or placeholder. It uses basic categorical features (like raw port numbers and protocols) purely to validate the ONNX C API integration, the inference pipeline, and the thread-safe ring buffer. It is *not* designed to be a statistically accurate anomaly detector in its current state.

## Overview
Sentinel-AI is a hybrid Network Intrusion Detection System (NIDS) built to analyze network traffic at wire-speed. The core achievement of this repository is the **C11 infrastructure**, which serves as an architectural bridge between low-level system programming (packet parsing, memory management) and dynamic AI execution.

The system is divided into two main components:
1. **The C Engine (`/engine`):** A high-performance, multithreaded packet sniffer and parser written in strict C11. It captures raw packets, extracts features via manual memory mapping, and executes AI inference natively.
2. **The AI Model (`/ai-model`):** A PyTorch-based Autoencoder trained on network traffic features, exported via ONNX for C integration.

## Core Architecture
* **Decoupled Producer-Consumer:** Uses a custom POSIX thread-safe ring buffer (with mutexes and condition variables) to decouple real-time packet capture (`libpcap`) from the heavy analysis phase.
* **Zero-Allocation Hot Path:** Ensures no dynamic memory allocations (`malloc`/`free`) occur inside the packet processing loop to minimize latency.
* **Manual Protocol Parsing:** Implements direct L2-L4 protocol parsing (Ethernet, IPv4, IPv6, TCP, UDP) using custom C structures and bitwise operations without relying on heavy external networking libraries.
* **Native AI Inference:** Integrates the ONNX Runtime C API to load the autoencoder and evaluate network anomalies natively within the C environment.

## Tech Stack
* **Language:** C11, Python 3.13
* **Libraries:** `libpcap`, `ONNX Runtime C API`, `pthread`
* **AI Ecosystem:** PyTorch, scikit-learn, Pandas
* **Build & Test:** CMake, Make, Custom Unity-inspired Test Framework

## Documentation & Notes
Additional context, architectural guidelines, and developer cheatsheets (covering libpcap and ONNX C API usage) can be found in the repository's `docs/` folder. Refer to `SENTINEL_AI_CONTEXT.md` for the original project charter and DoD (Definition of Done).

## Building the Engine

```bash
# Build the default Sniffer mode
make all

# Build and run the test suite
make test

# Run in Data Collection Mode (outputs to CSV)
sudo make train

# Run in Active Inference Mode (AI Guard)
sudo make guard
```