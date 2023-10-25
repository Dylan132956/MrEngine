#pragma once

#include "object_id_allocator.h"
#include <string>
#include <unordered_set>

namespace moonriver
{
    class FastObjectManager;
    class Object 
    {
    public:
        Object();
        ~Object();
        static FastObjectManager& GetObjectManager();
    protected:
        ObjectID   m_id{ k_invalid_gobject_id };
        std::string m_name;
    };

    class FastObjectManager
    {
    public:
        FastObjectManager();
        ~FastObjectManager();
        void AddObject(Object* p);
        void DeleteObject(Object* p);
        bool IsClear();
        size_t GetObjectNum();
    protected:
        std::unordered_set<Object*> ObjectHashTree;
        size_t m_uiObjectNum;
    };
}


