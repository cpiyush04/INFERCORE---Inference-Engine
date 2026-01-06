#include <iostream>
#include <vector>
#include <thread>
#include "queue.hpp"
#include "engine.hpp"

const int TOTAL_REQUESTS = 50;

int main() {
    std::cout << "Starting InferCore" << std::endl;

    ThreadSafeQueue<Request> global_queue;
    InferenceEngine engine(global_queue);

    // Start the Timer
    auto start_time = std::chrono::high_resolution_clock::now();

    // Launch multiple producer threads to simulate high request load
    std::vector<std::thread> producers;
    for (int i = 0; i < TOTAL_REQUESTS; ++i) {
        producers.emplace_back([&global_queue, i]() {

            // Small delay to simulate high throughput
            std::this_thread::sleep_for(std::chrono::microseconds(100)); 

            // Create a request with varying token sizes
            Request req{i, 5 + (i % 5), std::chrono::high_resolution_clock::now()};
            global_queue.push(req);
        });
    }

    // Wait for all producers to finish submitting requests
    for (auto& t : producers) t.join();

    // Wait until the queue becomes empty, meaning all requests have been consumed by the inference engine
    while (!global_queue.empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Give the worker a moment to finish the last batch logic
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Stop Timer
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end_time - start_time;

    // Benchmark results
    std::cout << "\n========================================\n";
    std::cout << "Total Requests: " << TOTAL_REQUESTS << "\n";
    std::cout << "Total Time: " << diff.count() << " s\n";
    std::cout << "Throughput: " << TOTAL_REQUESTS / diff.count() << " req/s\n";
    std::cout << "========================================\n";

    return 0;
}
