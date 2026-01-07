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

    // Returns whether shutdown has been requested
    bool is_stopped() {
        std::lock_guard<std::mutex> lock(mutex_);
        return stopped_;
    }

    // Pops up to `max_batch_size` elements into `out_batch`.
    size_t pop_batch(std::vector<T>& out_batch, size_t max_batch_size, int timeout_ms) {
        std::unique_lock<std::mutex> lock(mutex_);

        // Update wait condition to include timeout
        cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms), 
            [this] { return !queue_.empty() || stopped_; });

        // Check if shutdown is requested and there is no remaining work,
        // return immediately to allow worker threads to exit.
        if (stopped_ && queue_.empty()) {
            return 0;
        }

        size_t count = 0;
        while (!queue_.empty() && count < max_batch_size) {
            out_batch.push_back(queue_.front());
            queue_.pop();
            count++;
        }

        return count;
    }
    
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }
};

