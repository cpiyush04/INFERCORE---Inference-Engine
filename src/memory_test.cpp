#include <iostream>
#include <vector>
#include <memory>
#include "kvcache_handling.hpp"

// Configuration
const int users_count = 50;       // Enough users to expose memory scaling issues
const int prompt_len = 64;       // short chat prompt length

int main() {

    std::cout << "=== Memory usage test ===\n";

    // Simulate a Server holding user sessions
    std::vector<std::unique_ptr<KVCache>> active_sessions;
    
    std::cout << "[Test] simulating " << users_count << " users connecting...\n";
    
    for (int i = 0; i < users_count; ++i) {
        // User connects -> System allocates memory
        auto session = std::make_unique<KVCache>(i);
        
        // User provides 'prompt_len' tokens as input
        for (int t = 0; t < prompt_len; ++t) {
            session->add_token();
        }
        
        active_sessions.push_back(std::move(session));
    }

    // calculate total memory usage
    size_t total_reserved_mb = 0;
    size_t total_used_mb = 0;

    for (const auto& session : active_sessions) {
        total_reserved_mb += session->get_reserved_bytes();
        total_used_mb += session->get_used_bytes();
    }
    
    double reserved_mb = total_reserved_mb / (1024.0 * 1024.0);
    double used_mb = total_used_mb / (1024.0 * 1024.0);
    double fragmentation = 100.0 * (1.0 - (used_mb / reserved_mb));

    // Test Results
    std::cout << "\n[Results]\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Users Simulated:    " << users_count << "\n";
    std::cout << "Tokens per User:    " << prompt_len << "\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Total Memory RESERVED: " << reserved_mb << " MB\n";
    std::cout << "Total Memory USED:     " << used_mb << " MB\n";
    std::cout << "----------------------------------------\n";
    std::cout << "WASTED MEMORY:         " << fragmentation << " %\n";
    std::cout << "========================================\n";

    return 0;
}