# Building Source Code

[![CI](https://github.com/z900collector/z80-Assembler/actions/workflows/ci.yml/badge.svg)](https://github.com/z900collector/z80-Assembler/actions/workflows/ci.yml)   

The z80 Assembler is a basic C++ program that uses simple objects to parse a z80 assmbly language source file and output a bin file.

## Compiling the code
```
g++ -std=c++17 -Wall -Wextra -pedantic -Werror -g asm-v3.cc -o asm-v3
```

## Assembling a file

```
asm-v3 <input file> 


