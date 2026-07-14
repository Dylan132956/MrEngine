#pragma once
#include "_generated\serializer\meta_example.serializer.gen.h"
namespace moonriver{
    template<>
    Json Serializer::write(const BaseTest& instance){
        Json::object  ret_context;
        
        ret_context.insert_or_assign("int", Serializer::write(instance.m_int));
        Json::array m_int_vector_json;
        for (auto& item : instance.m_int_vector){
            m_int_vector_json.emplace_back(Serializer::write(item));
        }
        ret_context.insert_or_assign("int_vector",m_int_vector_json);
        
        return  Json(ret_context);
    }
    template<>
    BaseTest& Serializer::read(const Json& json_context, BaseTest& instance){
        assert(json_context.is_object());
        
        if(!json_context["int"].is_null()){
            Serializer::read(json_context["int"], instance.m_int);
        }
        if(!json_context["int_vector"].is_null()){
            assert(json_context["int_vector"].is_array());
            Json::array array_m_int_vector = json_context["int_vector"].array_items();
            instance.m_int_vector.resize(array_m_int_vector.size());
            for (size_t index=0; index < array_m_int_vector.size();++index){
                Serializer::read(array_m_int_vector[index], instance.m_int_vector[index]);
            }
        }
        return instance;
    }
    template<>
    Json Serializer::write(const Test1& instance){
        Json::object  ret_context;
        auto&&  json_context_0 = Serializer::write(*(moonriver::BaseTest*)&instance);
        assert(json_context_0.is_object());
        auto&& json_context_map_0 = json_context_0.object_items();
        ret_context.insert(json_context_map_0.begin() , json_context_map_0.end());
        ret_context.insert_or_assign("char", Serializer::write(instance.m_char));
        return  Json(ret_context);
    }
    template<>
    Test1& Serializer::read(const Json& json_context, Test1& instance){
        assert(json_context.is_object());
        Serializer::read(json_context,*(moonriver::BaseTest*)&instance);
        if(!json_context["char"].is_null()){
            Serializer::read(json_context["char"], instance.m_char);
        }
        return instance;
    }
    template<>
    Json Serializer::write(const Test2& instance){
        Json::object  ret_context;
        auto&&  json_context_0 = Serializer::write(*(moonriver::BaseTest*)&instance);
        assert(json_context_0.is_object());
        auto&& json_context_map_0 = json_context_0.object_items();
        ret_context.insert(json_context_map_0.begin() , json_context_map_0.end());
        Json::array m_test_base_array_json;
        for (auto& item : instance.m_test_base_array){
            m_test_base_array_json.emplace_back(Serializer::write(item));
        }
        ret_context.insert_or_assign("test_base_array",m_test_base_array_json);
        
        return  Json(ret_context);
    }
    template<>
    Test2& Serializer::read(const Json& json_context, Test2& instance){
        assert(json_context.is_object());
        Serializer::read(json_context,*(moonriver::BaseTest*)&instance);
        if(!json_context["test_base_array"].is_null()){
            assert(json_context["test_base_array"].is_array());
            Json::array array_m_test_base_array = json_context["test_base_array"].array_items();
            instance.m_test_base_array.resize(array_m_test_base_array.size());
            for (size_t index=0; index < array_m_test_base_array.size();++index){
                Serializer::read(array_m_test_base_array[index], instance.m_test_base_array[index]);
            }
        }
        return instance;
    }

}

