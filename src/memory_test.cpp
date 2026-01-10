#include <iostream>
#include <vector>
#include <memory>
#include "kvcache_handling.hpp"

// Configuration
const int users_count = 50;       // Enough users to expose memory scaling issues
const int tokens_to_generate = 64;       // short chat prompt length

int main() {

    std::cout << "=== Memory usage test ===\n";

    // // Simulate a Server holding user sessions
    // std::vector<std::unique_ptr<KVCache>> active_sessions;
    KVCacheManager kv_manager(1000);
    
    std::cout << "[Test] simulating " << users_count << " users connecting...\n";
    
    for (int i = 0; i < users_count; ++i) {
        for (int t = 0; t < tokens_to_generate; ++t) {
            // session->add_token();
            kv_manager.append_token(i);
        }

    }

    // calculate total memory usage
    size_t total_reserved_mb = 0;
    size_t total_used_mb = 0;

    for (int i = 0; i < users_count; ++i) {
        total_reserved_mb += kv_manager.get_reserved_bytes(i);
        total_used_mb += kv_manager.get_used_bytes(i);
    }
    
    double reserved_mb = total_reserved_mb / (1024.0 * 1024.0);
    double used_mb = total_used_mb / (1024.0 * 1024.0);
    double fragmentation = 100.0 * (1.0 - (used_mb / reserved_mb)); 

    // Test Results
    std::cout << "\n[Results]\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Users Simulated:    " << users_count << "\n";
    std::cout << "Tokens per User:    " << tokens_to_generate << "\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Total Memory RESERVED: " << reserved_mb << " MB\n";
    std::cout << "Total Memory USED:     " << used_mb << " MB\n";
    std::cout << "----------------------------------------\n";
    std::cout << "WASTED MEMORY:         " << fragmentation << " %\n";
    std::cout << "========================================\n";

    return 0;
}