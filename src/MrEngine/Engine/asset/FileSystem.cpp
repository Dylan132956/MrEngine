#include "asset/FileSystem.h"

#include <fstream>
#include <sstream>

namespace AsyncLoad {

FileReadResult LocalFileSystem::ReadAllBytes(const std::string& path) {
    FileReadResult result;

    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file) {
        result.error = "Failed to open file: " + path;
        return result;
    }

    file.seekg(0, std::ios::end);
    const std::ifstream::pos_type size = file.tellg();
    if (size < 0) {
        result.error = "Failed to get file size: " + path;
        return result;
    }

    file.seekg(0, std::ios::beg);
    result.data.resize(static_cast<std::size_t>(size));
    if (!result.data.empty()) {
        file.read(reinterpret_cast<char*>(&result.data[0]), size);
        if (!file) {
            result.data.clear();
            result.error = "Failed to read file: " + path;
            return result;
        }
    }

    result.success = true;
    return result;
}

} // namespace AsyncLoad
