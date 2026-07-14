#pragma once

#include <functional>
#include <string>

namespace AsyncLoad {

class AssetId {
public:
    AssetId() {}
    explicit AssetId(const std::string& value) : value_(value) {}
    explicit AssetId(std::string&& value) : value_(std::move(value)) {}

    const std::string& ToString() const { return value_; }
    bool Empty() const { return value_.empty(); }

    bool operator==(const AssetId& rhs) const { return value_ == rhs.value_; }
    bool operator!=(const AssetId& rhs) const { return !(*this == rhs); }

private:
    std::string value_;
};

struct AssetIdHash {
    std::size_t operator()(const AssetId& id) const {
        return std::hash<std::string>()(id.ToString());
    }
};

} // namespace AsyncLoad
