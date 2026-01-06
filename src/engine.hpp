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

    // Consumer loop
    void loop() {
        while (true) {

            // Empty request object to be filled
            Request req;

            bool active = request_queue_.wait_and_pop(req);

            // 2. If false, Break the loop immediately!
            if (!active) {

                // Shutdown was triggered
                std::cout << "[Worker] Shutdown signal received. Exiting.\n";
                break;
            }

            // Simulate request processing time based on input length
            int processing_time = req.input_token_count * 10; 
            
            std::cout << "[Worker] Processing Request ID: " << req.id 
                      << " (" << req.input_token_count << " tokens)..." << std::endl;
            
            // Artificial delay to simulate GPU/CPU compute
            std::this_thread::sleep_for(std::chrono::milliseconds(processing_time));

            // End-to-End latency from request creation
            auto end_time = std::chrono::high_resolution_clock::now();
            auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - req.start_time).count();

            std::cout << "[Worker] Finished ID: " << req.id 
                      << " | Latency: " << latency << "ms" << std::endl;
        }
    }
};

