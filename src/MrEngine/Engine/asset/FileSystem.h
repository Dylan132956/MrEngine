#pragma once

#include "asset/Result.h"

#include <string>

namespace AsyncLoad {

class IFileSystem {
public:
    virtual ~IFileSystem() {}
    virtual FileReadResult ReadAllBytes(const std::string& path) = 0;
};

class LocalFileSystem : public IFileSystem {
public:
    FileReadResult ReadAllBytes(const std::string& path) override;
};

} // namespace AsyncLoad
