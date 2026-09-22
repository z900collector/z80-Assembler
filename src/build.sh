#!/bin/bash

#g++ -std=c++17 -Wall -Wextra -pedantic -Werror -g asm.cc -o asm
#g++ -std=c++17 -Wall -Wextra -pedantic -Werror -g asm-v2.cc -o asm-v2

g++ -std=c++17 -Wall -Wextra -pedantic -Werror -g Assembler.cpp IInstructionSet.h Z80InstructionSet.cpp MOS6502InstructionSet.cpp  asm-v3.cpp  -o asm-v3
