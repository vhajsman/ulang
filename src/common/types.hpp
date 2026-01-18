#ifndef __ULANG_COM_TYPES_H
#define __ULANG_COM_TYPES_H

#define ULANG_TYPE_MAX_SZ 16

#include <cstddef>
#include <cstdint>
#include <string>

namespace ULang {
    enum class DataTypeKind {
        Int,
        Char
    };

    enum DataTypeFlags {
        SIGN     = 1 << 0,
        USER     = 1 << 1,
        INTEGRAL = 1 << 2,
        NUMERIC  = 1 << 3
    };

    struct DataType {
        std::string name;
        size_t size;
        uint8_t flags;
        DataTypeKind kind;
    };

    const ULang::DataType* resolveDataType(const std::string& typeName);

#define _FLAGS_UINT (DataTypeFlags::INTEGRAL | DataTypeFlags::NUMERIC)
#define _FLAGS_INT  (DataTypeFlags::SIGN | _FLAGS_UINT)

    inline const DataType TYPE_INT8  {"int8",  1, _FLAGS_INT, DataTypeKind::Int};
    inline const DataType TYPE_INT16 {"int16", 2, _FLAGS_INT, DataTypeKind::Int};
    inline const DataType TYPE_INT32 {"int32", 4, _FLAGS_INT, DataTypeKind::Int};
    inline const DataType TYPE_INT64 {"int64", 8, _FLAGS_INT, DataTypeKind::Int};

    inline const DataType TYPE_UINT8  {"uint8",  1, _FLAGS_UINT, DataTypeKind::Int};
    inline const DataType TYPE_UINT16 {"uint16", 2, _FLAGS_UINT, DataTypeKind::Int};
    inline const DataType TYPE_UINT32 {"uint32", 4, _FLAGS_UINT, DataTypeKind::Int};
    inline const DataType TYPE_UINT64 {"uint64", 8, _FLAGS_UINT, DataTypeKind::Int};

    inline const DataType TYPE_CHAR {"char", 1, DataTypeFlags::INTEGRAL, DataTypeKind::Char};
};

#endif