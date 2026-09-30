# Sentinel-AI: Multithreaded Network Intrusion Detection System

> 🚧 **Architectural Focus & Disclaimer**
> This project was built to explore low-level network programming, multithreading, and C-to-AI integration. 
> **Note on the AI Model:** The included PyTorch autoencoder is strictly a "toy model" or placeholder. It uses basic categorical features (like raw port numbers and protocols) purely to validate the ONNX API integration, the inference pipeline, and the thread-safe ring buffer. It is *not* designed to be a statistically accurate anomaly detector in its current state.

## Overview
Sentinel-AI is a hybrid Network Intrusion Detection System (NIDS) built to analyze network traffic at wire-speed. The core achievement of this repository is the low-level infrastructure, which serves as an architectural bridge between systems programming (packet parsing, memory management) and dynamic AI execution.

*Evolution Note: The project was originally built in strict C11 to master zero-allocation hot paths and bare-metal memory management. It is currently expanding with a Modern C++20 port to leverage RAII and zero-cost abstractions for safer concurrency.*

The system is divided into three main components:
1. **The Legacy C Engine (`/engine-c`):** A high-performance, multithreaded packet sniffer and parser written in strict C11. It captures raw packets, extracts features via manual memory mapping, and executes AI inference natively.
2. **The Modern C++ Engine (`/engine-cpp` - WIP):** The upcoming port using C++20 features (smart pointers, `std::mutex`, standard containers) for memory-safe protocol parsing and lock-free concurrency.
3. **The AI Model (`/ai-model`):** A PyTorch-based Autoencoder trained on network traffic features, exported via ONNX for native integration.

## Core Architecture
* **Decoupled Producer-Consumer:** Uses a custom thread-safe ring buffer to decouple real-time packet capture (`libpcap`) from the heavy analysis phase.
* **Zero-Allocation Hot Path:** Ensures no dynamic memory allocations occur inside the packet processing loop to minimize latency.
* **Manual Protocol Parsing:** Implements direct L2-L4 protocol parsing (Ethernet, IPv4, IPv6, TCP, UDP) using custom structures and bitwise operations without relying on heavy external networking libraries.
* **Native AI Inference:** Integrates the ONNX Runtime APIs to load the autoencoder and evaluate network anomalies natively.

## Tech Stack
* **Language:** C11, C++20 (WIP), Python 3.13
* **Libraries:** `libpcap`, `ONNX Runtime API`, `pthread` / `std::thread`
* **AI Ecosystem:** PyTorch, scikit-learn, Pandas
* **Build & Test:** CMake, Make, Custom Unity-inspired Test Framework

## Documentation & Notes
Additional context, architectural guidelines, and developer cheatsheets (covering libpcap and ONNX API usage) can be found in the repository's `docs/` folder. Refer to `SENTINEL_AI_CONTEXT.md` for the original project charter and DoD (Definition of Done).

## Building the Engine

To build and run the original C11 engine:

```bash
cd engine-c

# Build the default Sniffer mode
make all

# Build and run the test suite
make test

# Run in Data Collection Mode (outputs to CSV)
sudo make train

# Run in Active Inference Mode (AI Guard)
sudo make guard