#include "types.hpp"
#include <stdexcept>

namespace ULang {
    const ULang::DataType* resolveDataType(const std::string& typeName) {
        using namespace ULang;

        if(typeName == "int8")   return &TYPE_INT8;
        if(typeName == "int16")  return &TYPE_INT16;
        if(typeName == "int32")  return &TYPE_INT32;
        if(typeName == "int64")  return &TYPE_INT64;

        if(typeName == "uint8")  return &TYPE_UINT8;
        if(typeName == "uint16") return &TYPE_UINT16;
        if(typeName == "uint32") return &TYPE_UINT32;
        if(typeName == "uint64") return &TYPE_UINT64;

        if(typeName == "char")   return &TYPE_CHAR;
        if(typeName == "void")   return &TYPE_VOID;

        throw std::runtime_error("could not resolve type: " + typeName);
    }
};
