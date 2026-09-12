# SENTINEL AI - Technical Context & Standards

## 1. Project Charter
Sentinel AI is a hybrid Network Intrusion Detection System (NIDS).
**Primary Goal:** Educational mastery of Low-Level Systems, Memory Management, and AI Integration.
**Secondary Goal:** A functioning security tool.

## 2. Technical Stack
* **Language:** C11 (Strict ANSI compliance where possible).
* **Build System:** CMake.
* **Testing:** Unity (or CUnit) for Unit Testing.
* **Tooling:** Valgrind (Memcheck, Helgrind), GDB, Clang-Format.
* **Libraries:** `libpcap` (Capture), `ONNX Runtime C API` (Inference), `cJSON` (Serialization).

## 3. Architecture Guidelines (To be Refined by User)
* **The Engine (C):**
    * Must handle raw packet capture and protocol parsing (Eth -> IP -> TCP/UDP).
    * Must be non-blocking.
    * **Constraint:** No memory allocations (malloc) inside the hot-loop (packet processing path). Pre-allocate everything.
* **The Interface (React):**
    * Receives data via WebSockets.
* **The Bridge:**
    * How do C and React talk? (User to decide: Raw Sockets? libwebsockets? ZeroMQ?).

## 4. Definition of Done (DoD)
A feature is only considered "Complete" when:
1.  The implementation is written in C.
2.  **Unit Tests** exist and pass.
3.  `valgrind` reports 0 memory leaks and 0 race conditions.
4.  Code is commented in English (Doxygen style for headers).

## 5. Development Phases (Roadmap)
1.  **Scaffolding:** CMake setup, folder structure, testing framework setup.
2.  **The Sniffer:** Raw packet capture using `libpcap`.
3.  **The Parser:** Extracting features manually (bit shifting, struct casting).
4.  **The Brain:** Integrating ONNX Runtime.
5.  **The Reporter:** Serialization and IPC (Inter-Process Communication).
