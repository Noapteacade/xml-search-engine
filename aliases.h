#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "document.h"
#include <filesystem>
#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using TF = std::unordered_map<std::string, uint64_t>;
using TFSorted = std::vector<std::pair<std::string, uint64_t>>;
using TFIndex = std::unordered_map<std::string, Document>;
using Metadata = std::unordered_map<std::string, std::string>;
using json = nlohmann::json;