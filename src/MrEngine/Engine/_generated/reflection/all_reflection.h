#pragma once
#include "core/meta/reflection/reflection.h"
#include "_generated/serializer/all_serializer.h"
#include "_generated\reflection\meta_example.reflection.gen.h"

namespace moonriver{
namespace Reflection{
    void TypeMetaRegister::metaRegister(){
        TypeWrappersRegister::MetaExample();
    }
}
}

