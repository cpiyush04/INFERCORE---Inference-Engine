#pragma once
#include <vector>
#include <iostream>
#include <thread>
#include <chrono>
#include "queue.hpp"

class InferenceEngine {
    ThreadSafeQueue<Request>& request_queue_;
    std::thread worker_thread_;
    

public:
    InferenceEngine(ThreadSafeQueue<Request>& queue) 
        : request_queue_(queue) {
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
                if (req.input_token_count > max_tokens) max_tokens = req.input_token_count;
                std::cout << req.id << " ";
            }
            std::cout << "] (Compute Load: " << max_tokens << ")\n";

            // Simulate request processing time based on max token length in the batch
            int processing_time = max_tokens * 10; 
            
            // Artificial delay to simulate GPU/CPU compute
            std::this_thread::sleep_for(std::chrono::milliseconds(processing_time));

            auto end_time = std::chrono::high_resolution_clock::now();

            // Log tail latencies for each batch request
            for (const auto& req : batch) {
                auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - req.start_time).count();
                std::cout << "[Worker] Finished ID: " << req.id 
                      << " | Latency: " << latency << "ms" << std::endl;
            }
        }
    }
};

