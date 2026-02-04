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

    struct Operand {
        OperandType type;                               ///< operand type
        uint32_t data;
    };

    enum Opcode : uint8_t {
        NOP = 0x00,
        PUSH = 0x01,
        POP = 0x02,
        ADD = 0x03,
        SUB = 0x04,
        MUL = 0x05,
        DIV = 0x06,
        LD = 0x07,
        ST = 0x08,
        JMP = 0x09,
        JZ = 0x0A,
        CALL = 0x0B,
        RET = 0x0C,
        HALT = 0x0D,
        MOV = 0x0E
    };

    struct Instruction {
        Opcode opcode;
        std::vector<Operand> operands;
        size_t offset; // for disassembly

        /**
         * @brief Calculates the total size of instruction in memory, including opcode and operand type/size specifiers.
         * 
         * @return size_t 
         */
        size_t calcTotalSz() const {
            size_t size = sizeof(opcode);
            size += 1 + 1 + sizeof(uint32_t) * 2;
            return size;
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

        size_t getSize() const;
    };

    #pragma pack(push, 1)
    /**
     * @brief Bytecode header structure
     * 
     */
    struct BytecodeHeader {
        char     magic[6];          ///< "ULANG0"
        uint8_t  version_major;     ///< 1
        uint8_t  version_minor;     ///< 0
        uint8_t  endian;            ///< 0 = LE, 1 = BE
        uint8_t  word_size;         ///< 4 or 8
        uint16_t header_size;       ///< sizeof(BytecodeHeader)
        uint32_t flags;             ///< BytecodeFlags
        uint32_t code_offset;       ///< Code section offset
        uint32_t code_size;         ///< Code section size
        uint32_t data_offset;       ///< Data section offset
        uint32_t data_size;         ///< Data section size
        uint32_t meta_offset;       ///< Meta section offset
        uint32_t meta_size;         ///< Meta section size
        uint32_t checksum;
        uint8_t  checksum_type;     ///< Checksum type: 0 = none, 1 = CRC32
        uint64_t entry_offset;      ///< Offset of main function (relative to file start)
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
    /**
     * @brief Metadata symbol table entry
     * 
     */
    struct MetaSymbol {
        uint32_t name_offset;
        uint32_t type_id;
        uint32_t stack_offset;
        uint32_t flags;
    };

    /**
     * @brief Metadata type table entry
     * 
     */
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
     * @brief Validate the bytecode header
     * 
     * @param hdr header
     * @param file_size file size
     * @return true if header valid
     * @return false if header NOT valid
     */
    bool validateHeader(const BytecodeHeader& hdr, size_t file_size);

    /**
     * @brief Validate metadata structure
     * 
     * @param hdr header
     * @param meta bytecode meta header
     * @param file_size  file size
     * @return true if valid
     * @return false if NOT valid
     */
    bool validateMetaSection(const BytecodeHeader& hdr, const BytecodeMetaHeader& meta, size_t file_size);
    
    uint32_t addStringToPool(std::string& pool, const std::string& str);

    /**
     * @brief Parse the instruction
     * 
     * @param stream instruction stream
     * @return Instruction 
     */
    Instruction parseInstruction(BytecodeStream& stream);

    const char* opcodeToStr(Opcode op);
    const char* operandTypeToStr(OperandType t);
};

#endif
