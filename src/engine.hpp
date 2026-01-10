#pragma once
#include <vector>
#include <iostream>
#include <thread>
#include <chrono>
#include "queue.hpp"
#include "kvcache_handling.hpp"

class InferenceEngine {
    ThreadSafeQueue<Request>& request_queue_;
    std::thread worker_thread_;
    KVCacheManager kv_manager_;
    

public:
    InferenceEngine(ThreadSafeQueue<Request>& queue) 
        : request_queue_(queue) 
        , kv_manager_(1000) // Initialize KV Cache with 1000 blocks
    {
        // Start the background worker immediately
        worker_thread_ = std::thread(&InferenceEngine::loop, this);
    }

    ~InferenceEngine() {
        std::cout << "[Main] Destructor called. Stopping queue...\n";
    
        // Unblock the waiting thread
        request_queue_.shutdown();
        if (worker_thread_.joinable()) worker_thread_.join();
        std::cout << "[Main] Worker stopped. Exiting cleanly.\n";
    }

private:

    // Consumer loop (Batching Version)
    void loop() {

        // Batch Setup
        const size_t MAX_BATCH_SIZE = 10;
        const int BATCH_TIMEOUT_MS = 5; // Waiting time to fill the batch
    
        while (true) {

            std::vector<Request> batch;

            size_t count = request_queue_.pop_batch(batch, MAX_BATCH_SIZE, BATCH_TIMEOUT_MS);

            if (count == 0) {
                if (request_queue_.is_stopped()) {
                    std::cout << "[Worker] Shutdown signal received. Exiting.\n";
                    break;
                }
                
                continue;
            }

            // max token length governs the compute time in batch
            int max_tokens = 0;

            std::cout << "[Worker] Batch of " << count << ": IDs [ ";
            for (const auto& req : batch) {
                if (req.tokens_to_generate > max_tokens) max_tokens = req.tokens_to_generate;
                std::cout << req.id << " ";
            }
            std::cout << "] (Compute Load: " << max_tokens << ")\n";

            for (int step = 0; step < max_tokens; ++step) {
                
                bool active_requests = false;

                for (auto& req : batch) {
                    // Only process requests that haven't finished generation
                    if (step < req.tokens_to_generate) {
                        // Actually allocate memory!
                        kv_manager_.append_token(req.id);
                        active_requests = true;
                    }
                }

                // If all requests in batch are done early, break
                if (!active_requests) break;
                
                // Simulate Compute Latency
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            auto end_time = std::chrono::high_resolution_clock::now();

            // Log tail latencies for each batch request
            for (const auto& req : batch) {
                size_t mem_used_bytes = kv_manager_.get_used_bytes(req.id);
                auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - req.start_time).count();
                std::cout << "[Worker] Finished ID: " << req.id 
                      << " | Latency: " << latency << "ms" << std::endl << " | VRAM: " << mem_used_bytes / 1024 << " KB\n";
            }   
        }
    }
};

