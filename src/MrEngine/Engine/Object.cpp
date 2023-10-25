#include "Object.h"
#include <assert.h>

namespace moonriver
{
    FastObjectManager& Object::GetObjectManager()
    {
        static FastObjectManager ms_ObjectManager;
        return  ms_ObjectManager;
    }

    Object::Object()
    {
        m_id = ObjectIDAllocator::alloc();
        GetObjectManager().AddObject(this);
    }
    Object::~Object()
    {
        GetObjectManager().DeleteObject(this);
    }
//
// FastObjectManager
//
    FastObjectManager::FastObjectManager()
    {
        m_uiObjectNum = 0;
        ObjectHashTree.reserve(1000);
    }

    FastObjectManager::~FastObjectManager()
    {

    }

    void FastObjectManager::AddObject(Object* p)
    {
        assert(p);
        ObjectHashTree.insert(p);
        m_uiObjectNum++;
    }

    void FastObjectManager::DeleteObject(Object* p)
    {
        assert(p);
        ObjectHashTree.erase(p);
        m_uiObjectNum--;
    }

    bool FastObjectManager::IsClear()
    {
        return m_uiObjectNum == 0;
    }

    size_t FastObjectManager::GetObjectNum()
    {
        return m_uiObjectNum;
    }
}


