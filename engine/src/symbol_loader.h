#pragma once 

#include <fstream>
#include <filesystem>
#include <string> 
#include <unordered_set>

std::unordered_set<std::string> load_symbols(const std::filesystem::path& path);