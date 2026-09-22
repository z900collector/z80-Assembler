#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <cctype>
#include <stdexcept>
#include <algorithm>

#include "MOS6502InstructionSet.h"

MOS6502InstructionSet::MOS6502InstructionSet() : m_pc(0x0000)
{
}

int MOS6502InstructionSet::parseImmediate(const std::string& s)
{
        if (s.size() >= 2 && (s[0] == '$' ||
            (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))))
        {
            return std::stoi(s, nullptr, 16);
        }
        return std::stoi(s, nullptr, 10);
}



bool MOS6502InstructionSet::isLabelRef(const std::string& token)
    {
        if (token.empty()) return false;
        if (token[0] >= '0' && token[0] <= '9') return false;
        if (token[0] == '$') return false;
        return true;
    }

    int MOS6502InstructionSet::resolveLabel(const std::string& name)
    {
        std::string upper = toUpper(name);
        auto it = m_labels.find(upper);
        if (it == m_labels.end())
        {
            throw std::runtime_error("Undefined label: " + upper);
        }
        return it->second;
    }

    void MOS6502InstructionSet::setPC(int pc) { m_pc = pc; }
    int MOS6502InstructionSet::getPC() const { return m_pc; }
    void MOS6502InstructionSet::advancePC(int n) { m_pc += n; }

    void MOS6502InstructionSet::defineLabel(const std::string& name)
    {
        m_labels[toUpper(name)] = m_pc;
    }

    bool MOS6502InstructionSet::hasLabel(const std::string& name) const
    {
        return m_labels.count(toUpper(name)) > 0;
    }

    void MOS6502InstructionSet::reset()
    {
        m_labels.clear();
        m_pc = 0x0000;
    }

    void MOS6502InstructionSet::resetPC()
    {
        m_pc = 0x0000;
    }

    int MOS6502InstructionSet::instructionSize(const std::string& mnem, const std::string& op)
    {
        if (mnem == "ORG") return 0;

        if (mnem == "DB")
        {
            std::istringstream iss(op);
            std::string token;
            int count = 0;
            while (iss >> token) count++;
            return count;
        }

        if (mnem == "DW")
        {
            std::istringstream iss(op);
            std::string token;
            int count = 0;
            while (iss >> token) count++;
            return count * 2;
        }

        if (mnem == "DS")
        {
            return parseImmediate(op);
        }

        // Branch instructions: always 2 bytes
        if (mnem == "BCC" || mnem == "BCS" || mnem == "BEQ" ||
            mnem == "BNE" || mnem == "BMI" || mnem == "BPL" ||
            mnem == "BVC" || mnem == "BVS" || mnem == "BRA")
        {
            return 2;
        }

        // JMP: 3 bytes
        if (mnem == "JMP") return 3;

        // JSR: 3 bytes
        if (mnem == "JSR") return 3;

        // Implied / accumulator: 1 byte
        if (mnem == "NOP" || mnem == "RTS" || mnem == "RTI" ||
            mnem == "CLC" || mnem == "CLD" || mnem == "CLI" ||
            mnem == "CLV" || mnem == "SEC" || mnem == "SED" ||
            mnem == "SEI" || mnem == "PHA" || mnem == "PLA" ||
            mnem == "PHP" || mnem == "PLP" || mnem == "TAX" ||
            mnem == "TAY" || mnem == "TXA" || mnem == "TYA" ||
            mnem == "TXS" || mnem == "TSX" || mnem == "ASL A" ||
            mnem == "LSR A" || mnem == "ROL A" || mnem == "ROR A")
        {
            return 1;
        }

        // 1-byte: implied ops with no operand
        if (op.empty()) return 1;

        // Parse operand to determine addressing mode
        std::string upper = toUpper(op);

        // Immediate: #n → 2 bytes
        if (upper[0] == '#') return 2;

        // Relative (branch): 2 bytes (handled above)

        // Zero page: 2 bytes
        // Absolute: 3 bytes
        // Indirect: 3 bytes

        // Check for parentheses (indirect)
        if (upper[0] == '(')
        {
            // (zp,X) → 2 bytes, (abs),Y → 3 bytes
            if (upper.find("),Y") != std::string::npos) return 3;
            return 2;
        }

        // Check for X/Y suffix
        if (upper.find(",X") != std::string::npos ||
            upper.find(",Y") != std::string::npos)
        {
            // Zero page X/Y → 2 bytes, Absolute X/Y → 3 bytes
            std::string addr = upper.substr(0, upper.find(','));
            if (isHexAddr(addr) && addr.size() > 2) return 3;
            return 2;
        }

        // Plain address
        if (isHexAddr(upper))
        {
            if (upper.size() > 2) return 3; // absolute
            return 2; // zero page
        }

        // Default: 1 byte (accumulator ops like ASL A, LSR A, etc.)
        return 1;
    }

    std::vector<unsigned char>
    MOS6502InstructionSet::assemble(const std::string& mnem, const std::string& op)
    {
        // ── Pseudo-ops ──────────────────────────────────────────────────────
        if (mnem == "ORG")
        {
            m_pc = parseImmediate(op);
            return {};
        }

        if (mnem == "DB")
        {
            std::vector<unsigned char> result;
            std::istringstream iss(op);
            std::string token;
            while (iss >> token)
            {
                result.push_back(static_cast<unsigned char>(parseImmediate(token)));
            }
            return result;
        }

        if (mnem == "DW")
        {
            std::vector<unsigned char> result;
            std::istringstream iss(op);
            std::string token;
            while (iss >> token)
            {
                int val = parseImmediate(token);
                result.push_back(static_cast<unsigned char>(val & 0xFF));
                result.push_back(static_cast<unsigned char>((val >> 8) & 0xFF));
            }
            return result;
        }

        if (mnem == "DS")
        {
            int count = parseImmediate(op);
            return std::vector<unsigned char>(count, 0x00);
        }

        // ── Implied (1-byte, no operand) ────────────────────────────────────
        if (op.empty())
        {
            if (mnem == "NOP")  return {0xEA};
            if (mnem == "RTS")  return {0x60};
            if (mnem == "RTI")  return {0x40};
            if (mnem == "CLC")  return {0x18};
            if (mnem == "CLD")  return {0xD8};
            if (mnem == "CLI")  return {0x58};
            if (mnem == "CLV")  return {0xB8};
            if (mnem == "SEC")  return {0x38};
            if (mnem == "SED")  return {0xF8};
            if (mnem == "SEI")  return {0x78};
            if (mnem == "PHA")  return {0x48};
            if (mnem == "PLA")  return {0x68};
            if (mnem == "PHP")  return {0x08};
            if (mnem == "PLP")  return {0x28};
            if (mnem == "TAX")  return {0xAA};
            if (mnem == "TAY")  return {0xA8};
            if (mnem == "TXA")  return {0x8A};
            if (mnem == "TYA")  return {0x98};
            if (mnem == "TXS")  return {0x9A};
            if (mnem == "TSX")  return {0xBA};
        }

        // ── Branch instructions (2-byte, relative) ──────────────────────────
        if (mnem == "BRA") return branch(0x80, op);
        if (mnem == "BCC") return branch(0x90, op);
        if (mnem == "BCS") return branch(0xB0, op);
        if (mnem == "BEQ") return branch(0xF0, op);
        if (mnem == "BNE") return branch(0xD0, op);
        if (mnem == "BMI") return branch(0x30, op);
        if (mnem == "BPL") return branch(0x10, op);
        if (mnem == "BVC") return branch(0x50, op);
        if (mnem == "BVS") return branch(0x70, op);

        // ── JMP / JSR (3-byte absolute) ─────────────────────────────────────
        if (mnem == "JMP")
        {
            std::string upper = toUpper(trim(op));
            if (upper[0] == '(')
            {
                // JMP (abs) — indirect
                std::string addr = upper.substr(1, upper.find(')') - 1);
                int val = parseImmediate(trim(addr));
                return {0x6E,
                        static_cast<unsigned char>(val & 0xFF),
                        static_cast<unsigned char>((val >> 8) & 0xFF)};
            }
            int addr = parseImmediate(upper);
            return {0x4C,
                    static_cast<unsigned char>(addr & 0xFF),
                    static_cast<unsigned char>((addr >> 8) & 0xFF)};
        }

        if (mnem == "JSR")
        {
            int addr = parseImmediate(toUpper(trim(op)));
            return {0x20,
                    static_cast<unsigned char>(addr & 0xFF),
                    static_cast<unsigned char>((addr >> 8) & 0xFF)};
        }

        // ── Load / Store ────────────────────────────────────────────────────
        if (mnem == "LDA" || mnem == "STA" ||
            mnem == "LDX" || mnem == "STX" ||
            mnem == "LDY" || mnem == "STY")
        {
            return assembleLoadStore(mnem, op);
        }

        // ── ALU: ADC, SBC, AND, ORA, EOR, CMP ──────────────────────────────
        if (mnem == "ADC" || mnem == "SBC" || mnem == "AND" ||
            mnem == "ORA" || mnem == "EOR" || mnem == "CMP")
        {
            return assembleALU(mnem, op);
        }

        // ── Shift / Rotate: ASL, LSR, ROL, ROR ─────────────────────────────
        if (mnem == "ASL" || mnem == "LSR" || mnem == "ROL" || mnem == "ROR")
        {
            return assembleShift(mnem, op);
        }

        // ── INC / DEC ───────────────────────────────────────────────────────
        if (mnem == "INC" || mnem == "DEC")
        {
            return assembleIncDec(mnem, op);
        }

        throw std::runtime_error("Unknown mnemonic: " + mnem);
    }


std::string MOS6502InstructionSet::trim(const std::string& s)
    {
        size_t start = s.find_first_not_of(" \t");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t");
        return s.substr(start, end - start + 1);
    }

std::string MOS6502InstructionSet::toUpper(const std::string& s)
    {
        std::string r = s;
        std::transform(r.begin(), r.end(), r.begin(), ::toupper);
        return r;
    }

bool MOS6502InstructionSet::isHexAddr(const std::string& s)
    {
        if (s.empty()) return false;
        if (s[0] == '$') return true;
        if (s.size() >= 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) return true;
        // Bare hex like $1234 or just digits (assume hex for 3+ chars)
        if (s.size() >= 3)
        {
            for (char c : s)
            {
                if (!std::isxdigit(static_cast<unsigned char>(c))) return false;
            }
            return true;
        }
        return false;
    }

    // Parse an address from an operand string
    int MOS6502InstructionSet::parseAddr(const std::string& s)
    {
        std::string t = trim(s);
        if (t[0] == '#')
        {
            t = t.substr(1);
        }
        return parseImmediate(trim(t));
    }

    // ── Branch helper ───────────────────────────────────────────────────────

    std::vector<unsigned char> MOS6502InstructionSet::branch(unsigned char opcode, const std::string& op)
    {
        int target = resolveLabel(trim(op));
        int offset = target - (m_pc + 2);
        if (offset < -128 || offset > 127)
        {
            throw std::runtime_error("Branch out of range: " + std::to_string(offset));
        }
        return {opcode, static_cast<unsigned char>(static_cast<signed char>(offset))};
    }

    // ── Load / Store ────────────────────────────────────────────────────────

    // Opcode table: [register][addressing_mode]
    // Registers: A=0, X=1, Y=2
    // Modes: IMM=0, ZP=1, ZPX=2, ABS=3, ABSX=4, ABSY=5, IND=6, ZPI=7, ABSYI=8

    std::vector<unsigned char> MOS6502InstructionSet::assembleLoadStore(const std::string& mnem, const std::string& op)
    {
        std::string upper = toUpper(trim(op));
        int reg = 0; // A
        if (mnem == "LDX" || mnem == "STX") reg = 1;
        if (mnem == "LDY" || mnem == "STY") reg = 2;

        bool isLoad = (mnem == "LDA" || mnem == "LDX" || mnem == "LDY");

        // Determine addressing mode and extract value
        std::string addrStr;
        int mode;

        if (upper[0] == '#')
        {
            mode = 0; // immediate
            addrStr = upper.substr(1);
        }
        else if (upper[0] == '(')
        {
            // Indirect modes
            size_t closeParen = upper.find(')');
            std::string inner = upper.substr(1, closeParen - 1);
            std::string suffix = (closeParen + 1 < upper.size()) ? upper.substr(closeParen + 1) : "";

            if (suffix == ",X" || suffix == ",x")
            {
                mode = 7; // (zp,X)
                addrStr = trim(inner);
            }
            else if (suffix == ",Y" || suffix == ",y")
            {
                mode = 8; // (abs),Y
                addrStr = trim(inner);
            }
            else
            {
                mode = 6; // (abs) — JMP only, but handle for completeness
                addrStr = trim(inner);
            }
        }
        else if (upper.find(",X") != std::string::npos ||
                 upper.find(",x") != std::string::npos)
        {
            mode = 2; // zero page X
            addrStr = trim(upper.substr(0, upper.find(',')));
        }
        else if (upper.find(",Y") != std::string::npos ||
                 upper.find(",y") != std::string::npos)
        {
            mode = 5; // absolute Y
            addrStr = trim(upper.substr(0, upper.find(',')));
        }
        else
        {
            addrStr = upper;
            // Determine if zero page or absolute by value
            int val = parseImmediate(addrStr);
            if (val <= 0xFF) mode = 1; // zero page
            else mode = 3; // absolute
        }

        int val = parseImmediate(addrStr);

        // Look up opcode
        unsigned char opcode = lookupLoadStore(reg, isLoad, mode);

        std::vector<unsigned char> result;
        result.push_back(opcode);

        switch (mode)
        {
            case 0: // immediate
                result.push_back(static_cast<unsigned char>(val & 0xFF));
                break;
            case 1: // zero page
            case 2: // zero page X
            case 7: // (zp,X)
                result.push_back(static_cast<unsigned char>(val & 0xFF));
                break;
            case 3: // absolute
            case 4: // absolute X
            case 5: // absolute Y
            case 6: // (abs)
            case 8: // (abs),Y
                result.push_back(static_cast<unsigned char>(val & 0xFF));
                result.push_back(static_cast<unsigned char>((val >> 8) & 0xFF));
                break;
        }

        return result;
    }

    unsigned char MOS6502InstructionSet::lookupLoadStore(int reg, bool isLoad, int mode)
    {
        // LDA opcodes
        if (reg == 0)
        {
            if (isLoad)
            {
                switch (mode)
                {
                    case 0: return 0xA9; // LDA #n
                    case 1: return 0xA5; // LDA zp
                    case 2: return 0xB5; // LDA zp,X
                    case 3: return 0xAD; // LDA abs
                    case 4: return 0xBD; // LDA abs,X
                    case 5: return 0xB9; // LDA abs,Y
                    case 7: return 0xA1; // LDA (zp,X)
                    case 8: return 0xB1; // LDA (abs),Y
                }
            }
            else // STA
            {
                switch (mode)
                {
                    case 1: return 0x85; // STA zp
                    case 2: return 0x95; // STA zp,X
                    case 3: return 0x8D; // STA abs
                    case 4: return 0x9D; // STA abs,X
                    case 5: return 0x99; // STA abs,Y
                    case 7: return 0x81; // STA (zp,X)
                    case 8: return 0x91; // STA (abs),Y
                }
            }
        }
        // LDX / STX
        else if (reg == 1)
        {
            if (isLoad)
            {
                switch (mode)
                {
                    case 0: return 0xA2; // LDX #n
                    case 1: return 0xA6; // LDX zp
                    case 2: return 0xB6; // LDX zp,Y
                    case 3: return 0xAE; // LDX abs
                    case 4: return 0xBE; // LDX abs,Y
                }
            }
            else // STX
            {
                switch (mode)
                {
                    case 1: return 0x86; // STX zp
                    case 2: return 0x96; // STX zp,Y
                    case 3: return 0x8E; // STX abs
                }
            }
        }
        // LDY / STY
        else
        {
            if (isLoad)
            {
                switch (mode)
                {
                    case 0: return 0xA0; // LDY #n
                    case 1: return 0xA4; // LDY zp
                    case 2: return 0xB4; // LDY zp,X
                    case 3: return 0xAC; // LDY abs
                    case 4: return 0xBC; // LDY abs,X
                }
            }
            else // STY
            {
                switch (mode)
                {
                    case 1: return 0x84; // STY zp
                    case 2: return 0x94; // STY zp,X
                    case 3: return 0x8C; // STY abs
                }
            }
        }

        throw std::runtime_error("Unsupported addressing mode for load/store");
    }

    // ── ALU ─────────────────────────────────────────────────────────────────

    std::vector<unsigned char> MOS6502InstructionSet::assembleALU(const std::string& mnem, const std::string& op)
    {
        std::string upper = toUpper(trim(op));
	
        // Determine base opcodes
        unsigned char immOp = 0, zpOp = 0, zpxOp = 0,
                       absOp = 0, absyOp = 0,
                       zpiOp = 0, absyiOp = 0;

        if (mnem == "ADC")
        {
            immOp = 0x69; zpOp = 0x65; zpxOp = 0x75;
            absOp = 0x6D; absyOp = 0x79;
            zpiOp = 0x61; absyiOp = 0x71;
        }
        else if (mnem == "SBC")
        {
            immOp = 0xE9; zpOp = 0xE5; zpxOp = 0xF5;
            absOp = 0xED; absyOp = 0xF9;
            zpiOp = 0xE1; absyiOp = 0xF1;
        }
        else if (mnem == "AND")
        {
            immOp = 0x29; zpOp = 0x25; zpxOp = 0x35;
            absOp = 0x2D; absyOp = 0x39;
            zpiOp = 0x21; absyiOp = 0x31;
        }
        else if (mnem == "ORA")
        {
            immOp = 0x09; zpOp = 0x05; zpxOp = 0x15;
            absOp = 0x0D; absyOp = 0x19;
            zpiOp = 0x01; absyiOp = 0x11;
        }
        else if (mnem == "EOR")
        {
            immOp = 0x49; zpOp = 0x45; zpxOp = 0x55;
            absOp = 0x4D; absyOp = 0x59;
            zpiOp = 0x41; absyiOp = 0x51;
        }
        else if (mnem == "CMP")
        {
            immOp = 0xC9; zpOp = 0xC5; zpxOp = 0xD5;
            absOp = 0xCD; absyOp = 0xD9;
            zpiOp = 0xC1; absyiOp = 0xD1;
        }

        // Determine addressing mode
        std::string addrStr;
        unsigned char opcode;

        if (upper[0] == '#')
        {
            opcode = immOp;
            addrStr = upper.substr(1);
        }
        else if (upper[0] == '(')
        {
            size_t closeParen = upper.find(')');
            std::string inner = upper.substr(1, closeParen - 1);
            std::string suffix = (closeParen + 1 < upper.size()) ? upper.substr(closeParen + 1) : "";
            if (suffix == ",Y" || suffix == ",y")
            {
                opcode = absyiOp;
            }
            else
            {
                opcode = zpiOp;
            }
            addrStr = trim(inner);
        }
        else if (upper.find(",X") != std::string::npos ||
                 upper.find(",x") != std::string::npos)
        {
            opcode = zpxOp;
            addrStr = trim(upper.substr(0, upper.find(',')));
        }
        else if (upper.find(",Y") != std::string::npos ||
                 upper.find(",y") != std::string::npos)
        {
            opcode = absyOp;
            addrStr = trim(upper.substr(0, upper.find(',')));
        }
        else
        {
            addrStr = upper;
            int val = parseImmediate(addrStr);
            if (val <= 0xFF) opcode = zpOp;
            else opcode = absOp;
        }

        int val = parseImmediate(addrStr);

        std::vector<unsigned char> result;
        result.push_back(opcode);

        if (opcode == immOp)
        {
            result.push_back(static_cast<unsigned char>(val & 0xFF));
        }
        else if (opcode == zpOp || opcode == zpxOp || opcode == zpiOp)
        {
            result.push_back(static_cast<unsigned char>(val & 0xFF));
        }
        else
        {
            result.push_back(static_cast<unsigned char>(val & 0xFF));
            result.push_back(static_cast<unsigned char>((val >> 8) & 0xFF));
        }

        return result;
    }

    // ── Shift / Rotate ──────────────────────────────────────────────────────

    std::vector<unsigned char> MOS6502InstructionSet::assembleShift(const std::string& mnem, const std::string& op)
    {
        std::string upper = toUpper(trim(op));

        // Accumulator forms (1-byte)
        if (upper == "A")
        {
            if (mnem == "ASL") return {0x0A};
            if (mnem == "LSR") return {0x4A};
            if (mnem == "ROL") return {0x2A};
            if (mnem == "ROR") return {0x6A};
        }

        // Memory forms (2 or 3 bytes)
        unsigned char zpOp = 0, zpxOp = 0, absOp = 0; 

        if (mnem == "ASL")
        {
            zpOp = 0x06; zpxOp = 0x16; absOp = 0x0E; 
        }
        else if (mnem == "LSR")
        {
            zpOp = 0x46; zpxOp = 0x56; absOp = 0x4E;
        }
        else if (mnem == "ROL")
        {
            zpOp = 0x26; zpxOp = 0x36; absOp = 0x2E; 
        }
        else if (mnem == "ROR")
        {
            zpOp = 0x66; zpxOp = 0x76; absOp = 0x6E; 
        }

        std::string addrStr;
        unsigned char opcode;

        if (upper.find(",X") != std::string::npos ||
            upper.find(",x") != std::string::npos)
        {
            opcode = zpxOp;
            addrStr = trim(upper.substr(0, upper.find(',')));
        }
        else
        {
            addrStr = upper;
            int val = parseImmediate(addrStr);
            if (val <= 0xFF) opcode = zpOp;
            else opcode = absOp;
        }

        int val = parseImmediate(addrStr);
        std::vector<unsigned char> result;
        result.push_back(opcode);

        if (opcode == zpOp || opcode == zpxOp)
        {
            result.push_back(static_cast<unsigned char>(val & 0xFF));
        }
        else
        {
            result.push_back(static_cast<unsigned char>(val & 0xFF));
            result.push_back(static_cast<unsigned char>((val >> 8) & 0xFF));
        }

        return result;
    }

    // ── INC / DEC ───────────────────────────────────────────────────────────

    std::vector<unsigned char> MOS6502InstructionSet::assembleIncDec(const std::string& mnem, const std::string& op)
    {
        std::string upper = toUpper(trim(op));

        unsigned char zpOp = (mnem == "INC") ? 0xE6 : 0xC6;
        unsigned char zpxOp = (mnem == "INC") ? 0xF6 : 0xD6;
        unsigned char absOp = (mnem == "INC") ? 0xEE : 0xCE;

        std::string addrStr;
        unsigned char opcode;

        if (upper.find(",X") != std::string::npos ||
            upper.find(",x") != std::string::npos)
        {
            opcode = zpxOp;
            addrStr = trim(upper.substr(0, upper.find(',')));
        }
        else
        {
            addrStr = upper;
            int val = parseImmediate(addrStr);
            if (val <= 0xFF) opcode = zpOp;
            else opcode = absOp;
        }

        int val = parseImmediate(addrStr);
        std::vector<unsigned char> result;
        result.push_back(opcode);

        if (opcode == zpOp || opcode == zpxOp)
        {
            result.push_back(static_cast<unsigned char>(val & 0xFF));
        }
        else
        {
            result.push_back(static_cast<unsigned char>(val & 0xFF));
            result.push_back(static_cast<unsigned char>((val >> 8) & 0xFF));
        }

        return result;
    }
// End of File   
