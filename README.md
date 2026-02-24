# ULang JIT
Experimental bytecode JIT compiler, virtual machine and toolkit.

ULang JIT is a custom runtime designed around:
* constant-sized instructions
* explicit operand typing
* virtual register architecture
* separation of compiler and VM
* extensible instruction encoding

## ✨ Features
* Custom bytecode format
* Virtual register model (CPU-like semantics)
* Explicit operand metadata
* Disassembler tool
* Designed for future optimizations (jump tables, speculative lowering, etc.)
* Deterministic instruction encoding
* Bytecode disassembler (`bcdisasm`)
* Bytecode metadata + symbol table inspector (`bcdump`)
* Standalone virtual machine (`vm`)
* Designed for advanced compiler strategies (jump tables, lowering strategies, etc.)

## 🛠 Build
```shell
# install build deps
sudo pacman -Sy base-devel boost boost-libs git

git clone https://github.com/vhajsman/ulang.git --depth 1
cd ulang
make
```

This builds all the toolkit and stores the executables in `build` directory. That includes `compiler_bin, bcdisasm, bcdump, vm`.

## 📌 Design Goals
* Explicit over implicit
* Simple decoding
* Deterministic behavior
* VM clarity over clever compression
* Compiler-controlled optimization strategies

## 🚀 Roadmap
* AST builder and parser - **DONE**
* Variables and functions - **DONE**
* Expressions - **DONE**
* User IO
* Basic flow controls (conditions, loops)
* Type casts
* Arrays, objects, structures, classes

## Example
```ulang
fn int32 myFunction() {
    int32 x = 5;
    return x + 2;
}

int32 result = myFunction();
```
Compiles to:
```asm
JMP .start
myFunction:
    ST &x 05h   ; store literal 5 into local variable x
    ADD &x, 02h ; x = x + 2
    RET &x      ; return value stored in x
.start:
CALL &myFunction
ST &result, r20:FNR ; store value of function return register into result
```