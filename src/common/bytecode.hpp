#ifndef __ULANG_COM_BYTECODE_H
#define __ULANG_COM_BYTECODE_H

#include <cstdint> // uint8_t
#include <cstddef>
#include <string>
#include <vector>

#define ULANG_OP_COUNT 2
#define ULANG_OP_MAX_DATA_SZ 16

namespace ULang {
    enum class OperandType: uint8_t {
        OP_NULL         = 0b0000,    // instruction has no operand
        OP_IMMEDIATE    = 0b0001,    // immediate value
        OP_REFERENCE    = 0b0010,    // address in virtual memory
        OP_CONSTANT     = 0b0100,    // constant from the pool
        OP_REGISTER     = 0b1000     // internal register
    };

    typedef uint8_t operandMeta_t;

    struct Operand {
        ULang::operandMeta_t raw_meta;
        uint8_t* value;

        /**
         * @brief Get operand type
         * 
         * @return ULang::OperandType 
         */
        ULang::OperandType getType() const { 
            return static_cast<ULang::OperandType>((raw_meta & 0b11110000) >> 4); 
        }

        /**
         * @brief Get the size of operand data
         * 
         * @return size_t 
         */
        size_t getDataSz() const { 
            return raw_meta & 0b00001111; 
        }
    };

    enum class Opcode : uint8_t {
        NOP,
        PUSH,
        POP,
        ADD,
        SUB,
        MUL,
        DIV,
        LOAD,
        STORE,
        JMP,
        JZ,
        CALL,
        RET,
        HALT
    };

    struct Instruction {
        Opcode opcode;
        Operand opA; Operand opB;

        /**
         * @brief Calculates the total size of instruction in memory, including opcode and operand type/size specifiers.
         * 
         * @return size_t 
         */
        size_t calcTotalSz() const {
            return sizeof(opcode) + sizeof(operandMeta_t) * 2 + opA.getDataSz() + opB.getDataSz();
        }
    };

    class BytecodeStream {
        private:
        const uint8_t* data;
        size_t size;
        size_t offset;

        public:
        BytecodeStream(const uint8_t* buffer, size_t bufferSz);

        /**
         * @brief Reads a single byte from bytecode stream and prepares for reading another one.
         * 
         * @exception std::out_of_range
         * 
         * @return uint8_t 
         */
        uint8_t readByte();

        /**
         * @brief Reads n bytes from bytecode stream and prepares for reading another ones.
         * 
         * @exception std::out_of_range
         * 
         * @param n byte count
         * @return const uint8_t* a pointer to reading or NULL
         */
        const uint8_t* readBytes(size_t n);

        /**
         * @brief Reads a single byte from bytecode stream but does not prepare for reading another one.
         * 
         * @exception std::out_of_range
         * 
         * @return uint8_t 
         */
        uint8_t peekByte();

        /**
         * @brief Reads n bytes from bytecode stream but does not prepare for reading another ones.
         * 
         * @exception std::out_of_range
         * 
         * @param n byte count
         * @return const uint8_t* a pointer to reading or NULL
         */
        const uint8_t* peekBytes(size_t n);

        /**
         * @brief Reads a single byte from bytecode stream specific position but does not prepare for reading another one.
         * 
         * @exception std::out_of_range
         * 
         * @param pos byte index
         * @return uint8_t 
         */
        uint8_t peekPos(size_t pos);

        /**
         * @brief Get current stream offset
         * 
         * @return size_t offset
         */
        size_t tell() const;

        /**
         * @brief check if end is reached
         * 
         * @return true 
         * @return false 
         */
        bool eof() const;

        /**
         * @brief Makes stream go back to its begining
         * 
         */
        void reset();
    };

    #pragma pack(push, 1)
    struct BytecodeHeader {
        char     magic[6];          // "ULANG0"
        uint8_t  version_major;     // 1
        uint8_t  version_minor;     // 0
        uint8_t  endian;            // 0 = LE, 1 = BE
        uint8_t  word_size;         // 4 or 8
        uint16_t header_size;       // sizeof(BytecodeHeader)
        uint32_t flags;             // BytecodeFlags
        uint32_t code_offset;
        uint32_t code_size;
        uint32_t meta_offset;
        uint32_t meta_size;
        uint32_t checksum;
        uint8_t  checksum_type;     // 0 = none, 1 = CRC32
        uint64_t entry_ptr;
        uint8_t  reserved[7];
    };
    #pragma pack(pop)

    enum BytecodeFlags : uint32_t {
        BC_FLAG_DEBUG      = 1 << 0,
        BC_FLAG_STRIPPED   = 1 << 1,
        BC_FLAG_SIGNED_VM  = 1 << 2,
        BC_FLAG_OPTIMIZED  = 1 << 3
    };

    #pragma pack(push, 1)
    struct MetaSymbol {
        uint32_t name_offset;
        uint32_t type_id;
        uint32_t stack_offset;
        uint32_t flags;
    };

    struct MetaType {
        uint32_t name_offset;
        uint32_t size;
        uint32_t flags;
    };

    struct BytecodeMetaHeader {
        uint32_t symbol_count;
        uint32_t type_count;
        uint32_t string_pool_size;
    };
    #pragma pack(pop)

    enum MetaSymbolFlags : uint32_t {
        SYM_GLOBAL = 1 << 0,
        SYM_CONST  = 1 << 1,
        SYM_PARAM  = 1 << 2,
    };

    enum MetaTypeFlags : uint32_t {
        TYPE_SIGNED   = 1 << 0,
        TYPE_FLOAT    = 1 << 1,
        TYPE_POINTER  = 1 << 2,
    };

    struct MetaData {
        std::vector<MetaType> types;
        std::vector<MetaSymbol> symbols;
        std::string string_pool;
    };

    /**
     * @brief Parse the instruction
     * 
     * @param stream instruction stream
     * @return Instruction 
     */
    Instruction parseInstruction(BytecodeStream& stream);
};

#endif
