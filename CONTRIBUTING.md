# Contributing to Universal ANC

## Code of Conduct & Real-Time Safety Rules

All contributions to Universal ANC must strictly adhere to the following real-time audio constraints:

1. **Zero Dynamic Memory Allocation in Audio Threads**:
   Never invoke `malloc`, `free`, `new`, `delete`, `std::vector::push_back`, or string formatting inside audio processing callbacks (`process`, `processBlock`, `filter`). All buffers must be preallocated during initialization.
2. **Lock-Free Execution**:
   Never acquire a blocking mutex, lock, or perform thread synchronization inside the real-time audio path. Use lock-free atomics or message passing for UI and telemetry.
3. **Deterministic Unit Tests**:
   Any new DSP algorithm or filter must include deterministic unit tests with repeatable assertions (no flaky or unbounded assertions).
4. **Transparent Capability Reporting**:
   Never present simulated or digital results as validated acoustic attenuation at a human ear. Clearly separate digital simulation, electrical loopback, and calibrated acoustic fixture measurements.
