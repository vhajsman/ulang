#ifndef __ULANG_COM_BYTECODE_H
#define __ULANG_COM_BYTECODE_H

#include <cstdint> // uint8_t
#include <cstddef>
#include <stdexcept>

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

        size_t calcTotalSz() const {
            return sizeof(opcode) + sizeof(operandMeta_t) * 2 + opA.getDataSz() + opB.getDataSz();
        }
    };

    class BytecodeStream {
        private:
        const uint8_t* data;
        size_t offset;
        size_t size;

        public:
        BytecodeStream(const uint8_t* buffer, size_t bufferSz);

        uint8_t readByte();
        const uint8_t* readBytes(size_t n);

        uint8_t peekByte();
        const uint8_t* peekBytes(size_t n);
        uint8_t peekPos(size_t pos);

        size_t tell() const;
        bool eof() const;
        void reset() const;
    };
};

#endif
