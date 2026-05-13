#include "symbol_loader.h"

std::unordered_set<std::string> load_symbols(const std::filesystem::path& path) {
    std::unordered_set<std::string> symbols;
    std::ifstream in{path};
    if (!in) return symbols;

    std::string line;
    while (std::getline(in, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) {
            line.pop_back();
        }
        if (!line.empty()) symbols.insert(line);
    }
    return symbols;
}