#pragma once
#include "core\meta\meta_example.h"

namespace moonriver{
    class BaseTest;
    class Test1;
    class Test2;
namespace Reflection{
namespace TypeFieldReflectionOparator{
    class TypeBaseTestOperator{
    public:
        static const char* getClassName(){ return "BaseTest";}
        static void* constructorWithJson(const Json& json_context){
            BaseTest* ret_instance= new BaseTest;
            Serializer::read(json_context, *ret_instance);
            return ret_instance;
        }
        static Json writeByName(void* instance){
            return Serializer::write(*(BaseTest*)instance);
        }
        // base class
        static int getBaseTestBaseClassReflectionInstanceList(ReflectionInstance* &out_list, void* instance){
            int count = 0;
            
            return count;
        }
        // fields
        static const char* getFieldName_m_int(){ return "m_int";}
        static const char* getFieldTypeName_m_int(){ return "int";}
        static void set_m_int(void* instance, void* field_value){ static_cast<BaseTest*>(instance)->m_int = *static_cast<int*>(field_value);}
        static void* get_m_int(void* instance){ return static_cast<void*>(&(static_cast<BaseTest*>(instance)->m_int));}
        static bool isArray_m_int(){ return false; }
        static const char* getFieldName_m_int_vector(){ return "m_int_vector";}
        static const char* getFieldTypeName_m_int_vector(){ return "std::vector<int*>";}
        static void set_m_int_vector(void* instance, void* field_value){ static_cast<BaseTest*>(instance)->m_int_vector = *static_cast<std::vector<int*>*>(field_value);}
        static void* get_m_int_vector(void* instance){ return static_cast<void*>(&(static_cast<BaseTest*>(instance)->m_int_vector));}
        static bool isArray_m_int_vector(){ return true; }

        // methods
        
    };
}//namespace TypeFieldReflectionOparator
namespace ArrayReflectionOperator{
#ifndef ArraystdSSvectorLintPROperatorMACRO
#define ArraystdSSvectorLintPROperatorMACRO
    class ArraystdSSvectorLintPROperator{
        public:
            static const char* getArrayTypeName(){ return "std::vector<int*>";}
            static const char* getElementTypeName(){ return "int*";}
            static int getSize(void* instance){
                //todo: should check validation
                return static_cast<int>(static_cast<std::vector<int*>*>(instance)->size());
            }
            static void* get(int index,void* instance){
                //todo: should check validation
                return static_cast<void*>(&((*static_cast<std::vector<int*>*>(instance))[index]));
            }
            static void set(int index, void* instance, void* element_value){
                //todo: should check validation
                (*static_cast<std::vector<int*>*>(instance))[index] = *static_cast<int**>(element_value);
            }
    };
#endif //ArraystdSSvectorLintPROperator
}//namespace ArrayReflectionOperator

    void TypeWrapperRegister_BaseTest(){
        FieldFunctionTuple* field_function_tuple_m_int=new FieldFunctionTuple(
            &TypeFieldReflectionOparator::TypeBaseTestOperator::set_m_int,
            &TypeFieldReflectionOparator::TypeBaseTestOperator::get_m_int,
            &TypeFieldReflectionOparator::TypeBaseTestOperator::getClassName,
            &TypeFieldReflectionOparator::TypeBaseTestOperator::getFieldName_m_int,
            &TypeFieldReflectionOparator::TypeBaseTestOperator::getFieldTypeName_m_int,
            &TypeFieldReflectionOparator::TypeBaseTestOperator::isArray_m_int);
        REGISTER_FIELD_TO_MAP("BaseTest", field_function_tuple_m_int);
        FieldFunctionTuple* field_function_tuple_m_int_vector=new FieldFunctionTuple(
            &TypeFieldReflectionOparator::TypeBaseTestOperator::set_m_int_vector,
            &TypeFieldReflectionOparator::TypeBaseTestOperator::get_m_int_vector,
            &TypeFieldReflectionOparator::TypeBaseTestOperator::getClassName,
            &TypeFieldReflectionOparator::TypeBaseTestOperator::getFieldName_m_int_vector,
            &TypeFieldReflectionOparator::TypeBaseTestOperator::getFieldTypeName_m_int_vector,
            &TypeFieldReflectionOparator::TypeBaseTestOperator::isArray_m_int_vector);
        REGISTER_FIELD_TO_MAP("BaseTest", field_function_tuple_m_int_vector);

        
        
        ArrayFunctionTuple* array_tuple_stdSSvectorLintPR = new  ArrayFunctionTuple(
            &ArrayReflectionOperator::ArraystdSSvectorLintPROperator::set,
            &ArrayReflectionOperator::ArraystdSSvectorLintPROperator::get,
            &ArrayReflectionOperator::ArraystdSSvectorLintPROperator::getSize,
            &ArrayReflectionOperator::ArraystdSSvectorLintPROperator::getArrayTypeName,
            &ArrayReflectionOperator::ArraystdSSvectorLintPROperator::getElementTypeName);
        REGISTER_ARRAY_TO_MAP("std::vector<int*>", array_tuple_stdSSvectorLintPR);
        ClassFunctionTuple* class_function_tuple_BaseTest=new ClassFunctionTuple(
            &TypeFieldReflectionOparator::TypeBaseTestOperator::getBaseTestBaseClassReflectionInstanceList,
            &TypeFieldReflectionOparator::TypeBaseTestOperator::constructorWithJson,
            &TypeFieldReflectionOparator::TypeBaseTestOperator::writeByName);
        REGISTER_BASE_CLASS_TO_MAP("BaseTest", class_function_tuple_BaseTest);
    }namespace TypeFieldReflectionOparator{
    class TypeTest1Operator{
    public:
        static const char* getClassName(){ return "Test1";}
        static void* constructorWithJson(const Json& json_context){
            Test1* ret_instance= new Test1;
            Serializer::read(json_context, *ret_instance);
            return ret_instance;
        }
        static Json writeByName(void* instance){
            return Serializer::write(*(Test1*)instance);
        }
        // base class
        static int getTest1BaseClassReflectionInstanceList(ReflectionInstance* &out_list, void* instance){
            int count = 1;
            out_list = new ReflectionInstance[count];
            for (int i=0;i<count;++i){
               out_list[i] = TypeMetaDef(moonriver::BaseTest,static_cast<Test1*>(instance));
            }
            return count;
        }
        // fields
        static const char* getFieldName_m_char(){ return "m_char";}
        static const char* getFieldTypeName_m_char(){ return "char";}
        static void set_m_char(void* instance, void* field_value){ static_cast<Test1*>(instance)->m_char = *static_cast<char*>(field_value);}
        static void* get_m_char(void* instance){ return static_cast<void*>(&(static_cast<Test1*>(instance)->m_char));}
        static bool isArray_m_char(){ return false; }

        // methods
        
    };
}//namespace TypeFieldReflectionOparator


    void TypeWrapperRegister_Test1(){
        FieldFunctionTuple* field_function_tuple_m_char=new FieldFunctionTuple(
            &TypeFieldReflectionOparator::TypeTest1Operator::set_m_char,
            &TypeFieldReflectionOparator::TypeTest1Operator::get_m_char,
            &TypeFieldReflectionOparator::TypeTest1Operator::getClassName,
            &TypeFieldReflectionOparator::TypeTest1Operator::getFieldName_m_char,
            &TypeFieldReflectionOparator::TypeTest1Operator::getFieldTypeName_m_char,
            &TypeFieldReflectionOparator::TypeTest1Operator::isArray_m_char);
        REGISTER_FIELD_TO_MAP("Test1", field_function_tuple_m_char);

        
        
        
        ClassFunctionTuple* class_function_tuple_Test1=new ClassFunctionTuple(
            &TypeFieldReflectionOparator::TypeTest1Operator::getTest1BaseClassReflectionInstanceList,
            &TypeFieldReflectionOparator::TypeTest1Operator::constructorWithJson,
            &TypeFieldReflectionOparator::TypeTest1Operator::writeByName);
        REGISTER_BASE_CLASS_TO_MAP("Test1", class_function_tuple_Test1);
    }namespace TypeFieldReflectionOparator{
    class TypeTest2Operator{
    public:
        static const char* getClassName(){ return "Test2";}
        static void* constructorWithJson(const Json& json_context){
            Test2* ret_instance= new Test2;
            Serializer::read(json_context, *ret_instance);
            return ret_instance;
        }
        static Json writeByName(void* instance){
            return Serializer::write(*(Test2*)instance);
        }
        // base class
        static int getTest2BaseClassReflectionInstanceList(ReflectionInstance* &out_list, void* instance){
            int count = 1;
            out_list = new ReflectionInstance[count];
            for (int i=0;i<count;++i){
               out_list[i] = TypeMetaDef(moonriver::BaseTest,static_cast<Test2*>(instance));
            }
            return count;
        }
        // fields
        static const char* getFieldName_m_test_base_array(){ return "m_test_base_array";}
        static const char* getFieldTypeName_m_test_base_array(){ return "std::vector<Reflection::ReflectionPtr<BaseTest>>";}
        static void set_m_test_base_array(void* instance, void* field_value){ static_cast<Test2*>(instance)->m_test_base_array = *static_cast<std::vector<Reflection::ReflectionPtr<BaseTest>>*>(field_value);}
        static void* get_m_test_base_array(void* instance){ return static_cast<void*>(&(static_cast<Test2*>(instance)->m_test_base_array));}
        static bool isArray_m_test_base_array(){ return true; }

        // methods
        
    };
}//namespace TypeFieldReflectionOparator
namespace ArrayReflectionOperator{
#ifndef ArraystdSSvectorLReflectionSSReflectionPtrLBaseTestRROperatorMACRO
#define ArraystdSSvectorLReflectionSSReflectionPtrLBaseTestRROperatorMACRO
    class ArraystdSSvectorLReflectionSSReflectionPtrLBaseTestRROperator{
        public:
            static const char* getArrayTypeName(){ return "std::vector<Reflection::ReflectionPtr<BaseTest>>";}
            static const char* getElementTypeName(){ return "Reflection::ReflectionPtr<BaseTest>";}
            static int getSize(void* instance){
                //todo: should check validation
                return static_cast<int>(static_cast<std::vector<Reflection::ReflectionPtr<BaseTest>>*>(instance)->size());
            }
            static void* get(int index,void* instance){
                //todo: should check validation
                return static_cast<void*>(&((*static_cast<std::vector<Reflection::ReflectionPtr<BaseTest>>*>(instance))[index]));
            }
            static void set(int index, void* instance, void* element_value){
                //todo: should check validation
                (*static_cast<std::vector<Reflection::ReflectionPtr<BaseTest>>*>(instance))[index] = *static_cast<Reflection::ReflectionPtr<BaseTest>*>(element_value);
            }
    };
#endif //ArraystdSSvectorLReflectionSSReflectionPtrLBaseTestRROperator
}//namespace ArrayReflectionOperator

    void TypeWrapperRegister_Test2(){
        FieldFunctionTuple* field_function_tuple_m_test_base_array=new FieldFunctionTuple(
            &TypeFieldReflectionOparator::TypeTest2Operator::set_m_test_base_array,
            &TypeFieldReflectionOparator::TypeTest2Operator::get_m_test_base_array,
            &TypeFieldReflectionOparator::TypeTest2Operator::getClassName,
            &TypeFieldReflectionOparator::TypeTest2Operator::getFieldName_m_test_base_array,
            &TypeFieldReflectionOparator::TypeTest2Operator::getFieldTypeName_m_test_base_array,
            &TypeFieldReflectionOparator::TypeTest2Operator::isArray_m_test_base_array);
        REGISTER_FIELD_TO_MAP("Test2", field_function_tuple_m_test_base_array);

        
        
        ArrayFunctionTuple* array_tuple_stdSSvectorLReflectionSSReflectionPtrLBaseTestRR = new  ArrayFunctionTuple(
            &ArrayReflectionOperator::ArraystdSSvectorLReflectionSSReflectionPtrLBaseTestRROperator::set,
            &ArrayReflectionOperator::ArraystdSSvectorLReflectionSSReflectionPtrLBaseTestRROperator::get,
            &ArrayReflectionOperator::ArraystdSSvectorLReflectionSSReflectionPtrLBaseTestRROperator::getSize,
            &ArrayReflectionOperator::ArraystdSSvectorLReflectionSSReflectionPtrLBaseTestRROperator::getArrayTypeName,
            &ArrayReflectionOperator::ArraystdSSvectorLReflectionSSReflectionPtrLBaseTestRROperator::getElementTypeName);
        REGISTER_ARRAY_TO_MAP("std::vector<Reflection::ReflectionPtr<BaseTest>>", array_tuple_stdSSvectorLReflectionSSReflectionPtrLBaseTestRR);
        ClassFunctionTuple* class_function_tuple_Test2=new ClassFunctionTuple(
            &TypeFieldReflectionOparator::TypeTest2Operator::getTest2BaseClassReflectionInstanceList,
            &TypeFieldReflectionOparator::TypeTest2Operator::constructorWithJson,
            &TypeFieldReflectionOparator::TypeTest2Operator::writeByName);
        REGISTER_BASE_CLASS_TO_MAP("Test2", class_function_tuple_Test2);
    }
namespace TypeWrappersRegister{
    void MetaExample()
    {
        TypeWrapperRegister_BaseTest();
    TypeWrapperRegister_Test1();
    TypeWrapperRegister_Test2();
    }
}//namespace TypeWrappersRegister

}//namespace Reflection
}//namespace moonriver

