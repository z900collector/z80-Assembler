# z80-ASM

Build Status [![C++ Assembler CI](https://github.com/z900collector/z80-Assembler/actions/workflows/main.yml/badge.svg)](https://github.com/z900collector/z80-Assembler/actions/workflows/main.yml)

# Overview

The assembler is a C++ program designed to have a replacable instruction set. Its been expanded and modified to handle additional CPU types, at present Z80 and 6502 CPU's.

It defaults to Z80 assembly language but also handles 6502 source code when passed the --6502 argument on the command line.

I is a different design to my original OO based CPU32-Assembler project. More of a bruteforce style with lots of local if-then logic.


## Compiling

The main.yml file in .github/workflows needs to have the additonal src files added to it in order to build correctly.


## Local Builds

For local builds outside of Github, the build.sh file can be modified to compile additional code.

Build instructions are in "src" directory.
