#pragma once
#include <vector>
#include <map>
#include <stack>
#include <iostream>

// Configuration
const int BLOCK_SIZE = 16; 
const int EMB_DIM = 1024; 

// A Single KV cache block representation
struct KVCacheBlock {
    int id;               
    int num_tokens;       
    
    // Storage 16 tokens * 1024 floats
    std::vector<float> memory_block;

    KVCacheBlock(int block_id) : id(block_id), num_tokens(0) {
        memory_block.resize(BLOCK_SIZE * EMB_DIM, 0.0f);
    }

    bool is_full() const {
        return num_tokens >= BLOCK_SIZE;
    }
};

// Manages allocation and tracking of KV cache blocks
class KVCacheManager {
private:
    std::vector<KVCacheBlock> all_blocks_;        // Physical Heap
    std::stack<int> free_block_ids_;              // Free List
    std::map<int, std::vector<int>> page_tables_; // User id -> list of block ids

public:
    // Initialize the fixed-size memory pool
    KVCacheManager(size_t num_blocks) {
        std::cout << "[Memory] Initializing Paged Manager with " << num_blocks << " blocks...\n";
        
        all_blocks_.reserve(num_blocks);
        for (int i = 0; i < num_blocks; ++i) {
            all_blocks_.emplace_back(i);
            free_block_ids_.push(i);
        }
    }

    // Assign a free block to a sequence.
    int allocate_block(int seq_id) {
        if (free_block_ids_.empty()) return -1; // OOM

        int physical_id = free_block_ids_.top();
        free_block_ids_.pop();

        page_tables_[seq_id].push_back(physical_id);
        all_blocks_[physical_id].num_tokens = 0; // Reset block
        return physical_id;
    }

    // Append one token for a given sequence.
    // If the current block is full, allocate a new one.
    void append_token(int seq_id) {
        auto& table = page_tables_[seq_id];

        // If no blocks or last block is full, allocate new one
        if (table.empty() || all_blocks_[table.back()].is_full()) {
            if (allocate_block(seq_id) == -1) return; // Silent fail for test (OOM)
        }

        // Add to the active block
        int active_id = table.back();
        all_blocks_[active_id].num_tokens++;
    }

    // Total memory reserved for a sequence
    size_t get_reserved_bytes(int seq_id) {
        if (page_tables_.find(seq_id) == page_tables_.end()) return 0;
        return page_tables_[seq_id].size() * BLOCK_SIZE * EMB_DIM * sizeof(float);
    }

    // Actual memory used by tokens written so far
    size_t get_used_bytes(int seq_id) {
        if (page_tables_.find(seq_id) == page_tables_.end()) return 0;

        size_t total_tokens = 0;
        // Sum up tokens in every block owned by this user
        for (int block_id : page_tables_[seq_id]) {
            total_tokens += all_blocks_[block_id].num_tokens;
        }
        
        return total_tokens * EMB_DIM * sizeof(float);
    }
};