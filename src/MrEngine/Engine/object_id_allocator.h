#pragma once

#include <atomic>
#include <limits>

namespace moonriver
{
    using ObjectID = std::size_t;

    constexpr ObjectID k_invalid_gobject_id = std::numeric_limits<std::size_t>::max();

    class ObjectIDAllocator
    {
    public:
        static ObjectID alloc();

    private:
        static std::atomic<ObjectID> m_next_id;
    };
} // namespace moonriver
