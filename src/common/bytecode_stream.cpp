#include "bytecode.hpp"

namespace ULang {
    BytecodeStream::BytecodeStream(const uint8_t* buffer, size_t bufferSz) 
    : data(buffer), size(bufferSz), offset(0) {}

    uint8_t BytecodeStream::readByte() {
        if(this->offset >= this->size)
            throw std::out_of_range("BytecodeStream: EOF");

        return this->data[this->offset++];
    }

    const uint8_t* BytecodeStream::readBytes(size_t n) {
        if(this->offset + n > this->size)
            throw std::out_of_range("BytecodeStream: EOF");

        const uint8_t* ptr = &this->data[offset];
        this->offset += n;

        return ptr;
    }

    uint8_t BytecodeStream::peekByte() {
        if(this->offset >= this->size)
            throw std::out_of_range("BytecodeStream: EOF");

        return this->data[this->offset];
    }

    const uint8_t* BytecodeStream::peekBytes(size_t n) {
        if(this->offset + n > this->size)
            throw std::out_of_range("BytecodeStream: EOF");

        const uint8_t* ptr = &this->data[offset];
        return ptr;
    }

    uint8_t BytecodeStream::peekPos(size_t pos) {
        if(pos >= this->size)
            throw std::out_of_range("BytecodeStream: EOF");

        return this->data[pos];
    }

    size_t BytecodeStream::tell() const {
        return this->offset;
    }

    bool BytecodeStream::eof() const {
        return this->offset >= this->size;
    }

    void BytecodeStream::reset() {
        this->offset = 0;
    }
};