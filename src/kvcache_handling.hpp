#pragma once
#include <vector>
#include <iostream>

class KVCache {
public:
    
    // defining LLM config constants
    static const int max_input_len = 2048; // max context length allowed
    static const int emb_dim = 1024;  // size of one token vector
    
    // Simulated contiguous memory block
    std::vector<float> memory_block;
    
    // Tracking
    int current_token_count = 0;
    size_t allocated_bytes = 0;

    KVCache(int id) {

        // allocate pre-determined memory immediately
        size_t total_elements = max_input_len * emb_dim;

        memory_block.resize(total_elements, 0.0f);
        
        // memory reserved for use to a single user
        allocated_bytes = total_elements * sizeof(float);
        
        // Note: We haven't stored any real data yet!
        current_token_count = 0;
    }

    // Simulate the user generating 1 token
    void add_token() {
        if (current_token_count < max_input_len) {
            current_token_count++;
        }
    }
    
    // Metrics
    size_t get_used_bytes() const {

        // memory that was actually used
        return current_token_count * emb_dim * sizeof(float);
    }
    
    size_t get_reserved_bytes() const {

        // memory that was reserved upfront
        return allocated_bytes;
    }
};