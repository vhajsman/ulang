#ifndef __ULANG_COM_BYTECODE_H
#define __ULANG_COM_BYTECODE_H

#include <cstdint> // uint8_t
#include <cstddef>

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

    /**
     * @brief Parse the instruction
     * 
     * @param stream instruction stream
     * @return Instruction 
     */
    Instruction parseInstruction(BytecodeStream& stream);
};

#endif
