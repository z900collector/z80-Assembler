#include "mos6502_iset.h"
#include <sstream>
#include <cctype>
#include <stdexcept>
#include <algorithm>

MOS6502InstructionSet::MOS6502InstructionSet()
    : m_pc(0x0000)
{
}

int MOS6502InstructionSet::parseImmediate(const std::string& s)
{
    // ... (all the method bodies from before, with MOS6502InstructionSet:: prefix)
}

// ... (rest of the implementations)   
