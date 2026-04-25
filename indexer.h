#pragma once
#include <string>
#include <filesystem>
#include "aliases.h"

std::string extract_documentation(const std::string& file_path);
Document generate_tf(const std::string& content);
void dump_index(const TFIndex& index, const Metadata& metadata, const fs::path& path);
bool load_index(TFIndex& index, const fs::path& path);