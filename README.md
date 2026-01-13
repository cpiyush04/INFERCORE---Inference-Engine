# INFERCORE
A High Performance Inference Engine (C++)
**Author:** Piyush Chandra

InferCore is a from-scratch inference engine written in C++, built to understand the low-level systems engineering behind modern AI servers like vLLM and TGI. Rather than using existing frameworks, this project reimplements two core serving mechanisms—**KV cache management** and **request batching**—to study their impact on throughput, latency, and memory efficiency.

The engine simulates the lifecycle of LLM inference requests and focuses on solving two fundamental bottlenecks in LLM serving:
- **Memory fragmentation in KV cache allocation**
- **Scheduling latency due to inefficient batching**

---

## 📉 Benchmarks & Results
The engine was benchmarked at each stage of development to measure how each architectural change affected performance.

| Architecture Stage | Throughput | Latency (Total) | Improvement |
|-------------------|------------|------------------|-------------|
| Serial Execution | 12.37 req/s | 4.04 s | Baseline |
| Naive Batching | 73.27 req/s | 0.68 s | ~6× speedup |
| Paged Attention Integrated | 59.48 req/s | 0.84 s | ~5× speedup + 0% memory waste |

### Memory Efficiency
- **Naive Allocation:** 96.8% VRAM wasted  
  *(allocating max sequence length for every request)*
- **Paged Attention:** 0% VRAM wasted  
  *(allocating 16-token blocks dynamically)*

---

## 🛠️ The Engineering Journey

This project evolved incrementally. Each phase exposed a bottleneck that motivated the next redesign.

### Phase 1 — Serial Request Processing (The Naive Start)
The most straightforward way to serve incoming LLM requests is to process them one at a time. The initial implementation used a simple **producer–consumer queue**.

**Design**
- Incoming requests stored in a thread-safe queue
- A single worker processes one request fully before moving to the next

**What Went Wrong**
- No overlap between requests
- GPU idle during request setup and I/O
- Throughput scaled very poorly

---

### Phase 2 — Naive Batching

To improve throughput, batching was introduced.

**Design Change**
- This simulates batched token generation, not actual GPU kernels
- The engine collects up to 10 requests
- Tokens for all requests are generated in parallel
- This simulates batched matrix multiplication during inference

**Impact**
- Throughput jumped to **73.27 req/s**
- Latency dropped significantly

**New Challenges Observed**
- Memory was allocated for the **worst-case sequence length**
- Most requests used only a small fraction of the reserved KV cache

This resulted in extremely high memory waste.

### Phase 3 — Solving the Memory Bottleneck (Paged Attention)

To quantify the issue, a memory benchmark was run with 50 simulated users.

**Observation**
- **Reserved:** 400 MB  
- **Actually used:** 12.5 MB  
- **Wasted:** 96.8%

**Solution: Paged KV Cache Manager**
- KV cache split into fixed-size **16-token blocks**
- Logical token positions mapped to physical blocks using a page table
- Blocks allocated only when needed

This design mirrors OS-style virtual memory management concepts, applied to KV cache tensors.

**Result**
- **Memory waste reduced to 0%**
- Allocation became proportional to actual sequence length

---

### Phase 4 — Final Integration

In the final stage, the **batching scheduler** and **paged memory manager** were fully integrated.

**What Changed**
- Every token generation performs a real page lookup
- Memory allocation and scheduling decisions occur together
- The system reflects realistic inference-time behavior

**Final Performance**
- **Throughput:** ~60 requests/second
- **Improvement:** ~5× over serial execution
- **Memory waste:** 0%

Despite the added complexity of dynamic memory management, performance remained high while achieving production-grade memory efficiency.

---

## 💻 Tech Stack

- **Language:** C++ (standard library only)
- **Concurrency:** `std::thread`, `std::mutex`, `std::condition_variable`
- **Memory Management:** Custom paged KV cache using
  - Stack-based free list
  - Page table for logical → physical mapping

---

## 💻 Getting Started

### Prerequisites
* C++ Compiler (GCC, Clang, or MSVC) supporting C++17
* CMake (Version 3.10 or higher)

1.  **Clone the repository**

2.  **Compile the project**
    ```bash
    mkdir build && cd build
    cmake ..
    cmake --build .
    ```
### Running the Engine

To run the main inference engine simulation:
```bash
./liteserve
```

To run the standalone memory fragmentation test:
```bash
./memory_test
```

