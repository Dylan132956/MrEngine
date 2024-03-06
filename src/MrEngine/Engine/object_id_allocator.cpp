#include "object_id_allocator.h"
#include "Debug.h"

namespace moonriver
{
    std::atomic<ObjectID> ObjectIDAllocator::m_next_id {0};

    ObjectID ObjectIDAllocator::alloc()
    {
        std::atomic<ObjectID> new_object_ret = {m_next_id.load()};
        m_next_id++;
        if (m_next_id >= k_invalid_gobject_id)
        {
            Log("gobject id overflow");
        }

        return new_object_ret;
    }

} // namespace Piccolo
