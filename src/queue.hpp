#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <optional>

// Simple structure representing an LLM inference request
struct Request {
    int id; // Unique ID for the user's request
    int input_token_count; // length of input tokens

    // added timestamp to track request creation time
    std::chrono::high_resolution_clock::time_point start_time;
};

template<typename T>

class ThreadSafeQueue {
private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    bool stopped_ = false;

public:

    // push new incoming request into the queue
    void push(T value) {
        {
            // using a lock guard because we need it for this block only
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(value);
        }

         // waking up one waiting worker thread after unlocking
        cv_.notify_one();
    }

    // Signal all workers to stop
    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopped_ = true;
        }
        cv_.notify_all();
    }

    // Blocks until data is available or shutdown is triggered
    bool wait_and_pop(T& out_value) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        // Wake up if we have data OR if we were told to stop
        cv_.wait(lock, [this] { 
            return !queue_.empty() || stopped_; 
        });


        if (stopped_ && queue_.empty()) {
            return false; // Signal to the worker to quit
        }

        out_value = queue_.front();
        queue_.pop();
        return true;
    }
    
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }
};

