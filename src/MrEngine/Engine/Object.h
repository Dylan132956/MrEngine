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
        virtual ~Object();
        const std::string& GetName() const { return m_name; }
        void SetName(const std::string& name) { m_name = name; }
        size_t GetId() const {
            return m_id;
        }
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


