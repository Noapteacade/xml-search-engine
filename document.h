#pragma once
#include <unordered_map>
#include <string>
#include <vector>
struct Document {
    std::unordered_map<std::string, uint64_t> freq;
    std::vector<std::pair<std::string, uint64_t>> sorted;
    uint64_t total_terms = 0;
};