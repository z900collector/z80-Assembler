# Building Source Code

[![C++ Assembler CI](https://github.com/z900collector/z80-Assembler/actions/workflows/main.yml/badge.svg)](https://github.com/z900collector/z80-Assembler/actions/workflows/main.yml)

The z80 Assembler is a basic C++ program that uses simple objects to parse a z80 assmbly language source file and output a bin file.

## Compiling the code
```
g++ -std=c++17 -Wall -Wextra -pedantic -Werror -g Assembler.cpp IInstructionSet.h Z80InstructionSet.cpp MOS6502InstructionSet.cpp  asm-v3.cpp  -o asm-v3
```

## Assembling a file

```
asm-v3 <input file> 


