/*
 * z80 Assembler V3 using basic C++
 *
 * Compile cc file using:
 *
 * g++ -std=c++17 -Wall -Wextra -pedantic -Werror -g asm-v3.cc -o asm-v3
 *
 * Partly AI generated, manually edited and updated: S Young 2026
 * GITHUB Workflow added to project
 *
 */

#include "Z80InstructionSet.h"


#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <cctype>
#include <stdexcept>
#include <algorithm>
#include <functional>



Z80InstructionSet::Z80InstructionSet(): m_pc(0x0000)
{
}



void Z80InstructionSet::reset()
{
    m_labels.clear();
    m_pc = 0x0000;
}

void Z80InstructionSet::resetPC()
{
    m_pc = 0x0000;
}

int Z80InstructionSet::instructionSize(const std::string& mnem, const std::string& op)
{
    // ── Pseudo-ops ──────────────────────────────────────────────────────────
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

    // ── 3-byte: JP, CALL, LD 16-bit,nn, LD (nn),A, LD A,(nn) ───────────────
    if (mnem == "JP" || mnem == "CALL") return 3;

    if (mnem == "LD")
    {
        if (op.find(',') != std::string::npos)
        {
            size_t comma = op.find(',');
            std::string dst = toUpper(trim(op.substr(0, comma)));
            std::string src = toUpper(trim(op.substr(comma + 1)));

            // LD (nn),A / LD A,(nn)
            if ((dst == "A" && src[0] == '(') ||
                (dst[0] == '(' && src == "A"))
            {
                // Check if it's (nn) not (HL)/(BC)/(DE)
                if (src != "(HL)" && src != "(BC)" && src != "(DE)" &&
                    dst != "(HL)" && dst != "(BC)" && dst != "(DE)")
                {
                    return 3;
                }
            }

            // LD 16-bit, nn
            if (isReg16(dst) && !isReg16(src) &&
                src.find('(') == std::string::npos)
            {
                return 3;
            }
        }
        return 1; // default: reg→reg, (HL),A, etc.
    }

    // ── 2-byte: JR, DJNZ, LD r,n, LD (HL),n, CB-prefixed, IN/OUT (n) ───────
    if (mnem == "JR" || mnem == "DJNZ") return 2;

    if (mnem == "LD" && op.find(',') != std::string::npos)
    {
        size_t comma = op.find(',');
        std::string dst = toUpper(trim(op.substr(0, comma)));
        std::string src = toUpper(trim(op.substr(comma + 1)));

        // LD r, n  (8-bit reg, immediate)
        if (isReg8(dst) && !isReg8(src) && src.find('(') == std::string::npos)
        {
            return 2;
        }
        // LD (HL), n
        if (dst == "(HL)" && src.find('(') == std::string::npos)
        {
            return 2;
        }
    }

    if (mnem == "IN" || mnem == "OUT")
    {
        if (op.find(',') == std::string::npos) return 2;
        return 1; // IN (C) / OUT (C),A
    }

    // CB-prefixed: RLC, RRC, RL, RR, SLA, SRA, SRL, BIT, SET, RES
    if (mnem == "RLC" || mnem == "RRC" || mnem == "RL" || mnem == "RR" ||
        mnem == "SLA" || mnem == "SRA" || mnem == "SRL" ||
        mnem == "BIT" || mnem == "SET" || mnem == "RES")
    {
        return 2;
    }

    // ── 1-byte: everything else ─────────────────────────────────────────────
    return 1;
}   



int Z80InstructionSet::parseImmediate(const std::string& s)
    {
        if (s.size() >= 2 && (s[0] == '$' ||
            (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))))
        {
            return std::stoi(s, nullptr, 16);
        }
        return std::stoi(s, nullptr, 10);
    }

bool Z80InstructionSet::isLabelRef(const std::string& token)
    {
        // A label ref is alphabetic (not a number or hex)
        if (token.empty()) return false;
        if (token[0] >= '0' && token[0] <= '9') return false;
        if (token[0] == '$') return false;
        return true;
}

int Z80InstructionSet::resolveLabel(const std::string& name)
    {
        std::string upper = toUpper(name);
        auto it = m_labels.find(upper);
        if (it == m_labels.end())
        {
            throw std::runtime_error("Undefined label: " + upper);
        }
        return it->second;
    }

    void Z80InstructionSet::setPC(int pc) { m_pc = pc; }



    int Z80InstructionSet::getPC() const { return m_pc; }



    void Z80InstructionSet::advancePC(int n) { m_pc += n; }




    void Z80InstructionSet::defineLabel(const std::string& name)
    {
        m_labels[toUpper(name)] = m_pc;
    }




bool Z80InstructionSet::hasLabel(const std::string& name) const
{
	return m_labels.count(toUpper(name)) > 0;
}


std::vector<unsigned char> Z80InstructionSet::assemble(const std::string& mnem, const std::string& op)
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

        // ── 1-byte instructions ─────────────────────────────────────────────
        if (mnem == "NOP")  return {0x00};
        if (mnem == "HALT") return {0x76};
        if (mnem == "RET")  return {0xC9};
        if (mnem == "RETN") return {0xC5};
        if (mnem == "RETI") return {0x45};
        if (mnem == "DAA")  return {0x27};
        if (mnem == "CPL")  return {0x2F};
        if (mnem == "CCF")  return {0x3F};
        if (mnem == "SCF")  return {0x37};
        if (mnem == "DI")   return {0xF3};
        if (mnem == "EI")   return {0xFB};
        if (mnem == "EXAF") return {0x08};
        if (mnem == "EXX")  return {0xD9};

        // ── RST ─────────────────────────────────────────────────────────────
        if (mnem == "RST")
        {
            int addr = parseImmediate(op);
            return {static_cast<unsigned char>(0xC7 + (addr >> 3))};
        }

        // ── JP / CALL / JR / DJNZ ───────────────────────────────────────────
        if (mnem == "JP" || mnem == "CALL")
        {
            int addr = resolveLabel(op);
            unsigned char prefix = (mnem == "JP") ? 0xC3 : 0xCD;
            return {prefix,
                    static_cast<unsigned char>(addr & 0xFF),
                    static_cast<unsigned char>((addr >> 8) & 0xFF)};
        }

        if (mnem == "JR" || mnem == "DJNZ")
        {
            int target = resolveLabel(op);
            int offset = target - (m_pc + 2);
            unsigned char opcode = (mnem == "JR") ? 0x18 : 0x10;
            return {opcode,
                    static_cast<unsigned char>(static_cast<signed char>(offset))};
        }

        // ── PUSH / POP ──────────────────────────────────────────────────────
        if (mnem == "PUSH" || mnem == "POP")
        {
            int idx = reg16Index(parseReg16(op));
            unsigned char base = (mnem == "PUSH") ? 0xC5 : 0xF5;
            return {static_cast<unsigned char>(base + (idx & 0x04))};
        }

        // ── EX ──────────────────────────────────────────────────────────────
        if (mnem == "EX")
        {
            std::string clean = removeParens(op);
            if (toUpper(clean) == "DE,HL") return {0xEB};
            if (toUpper(clean) == "SP,HL") return {0xE3};
            throw std::runtime_error("Unknown EX operand: " + op);
        }

        // ── IN / OUT ────────────────────────────────────────────────────────
        if (mnem == "IN")
        {
            if (op.find(',') != std::string::npos) return {0xD8};
            int port = parseImmediate(op);
            return {0xDB, static_cast<unsigned char>(port)};
        }
        if (mnem == "OUT")
        {
            if (op.find(',') != std::string::npos) return {0xC8};
            int port = parseImmediate(op);
            return {0xD3, static_cast<unsigned char>(port)};
        }

        // ── LD ──────────────────────────────────────────────────────────────
        if (mnem == "LD")
        {
            return assembleLD(op);
        }

        // ── ALU ─────────────────────────────────────────────────────────────
        if (mnem == "ADD" || mnem == "SUB" || mnem == "AND" ||
            mnem == "OR" || mnem == "XOR")
        {
            return assembleALU(mnem, op);
        }

        // ── INC / DEC ───────────────────────────────────────────────────────
        if (mnem == "INC" || mnem == "DEC")
        {
            return assembleIncDec(mnem, op);
        }

        // ── Rotate / Shift ──────────────────────────────────────────────────
        if (mnem == "RLC" || mnem == "RRC" || mnem == "RL" || mnem == "RR" ||
            mnem == "SLA" || mnem == "SRA" || mnem == "SRL")
        {
            return assembleRotate(mnem, op);
        }

        // ── BIT / SET / RES ─────────────────────────────────────────────────
        if (mnem == "BIT" || mnem == "SET" || mnem == "RES")
        {
            return assembleBit(mnem, op);
        }

        throw std::runtime_error("Unknown mnemonic: " + mnem);
    }


    Z80InstructionSet::Reg8 Z80InstructionSet::parseReg8(const std::string& s)
    {
        std::string u = toUpper(s);
        if (u == "B") return Reg8::B;
        if (u == "C") return Reg8::C;
        if (u == "D") return Reg8::D;
        if (u == "E") return Reg8::E;
        if (u == "H") return Reg8::H;
        if (u == "L") return Reg8::L;
        if (u == "A") return Reg8::A;
        if (u == "F") return Reg8::F;
        throw std::runtime_error("Unknown 8-bit register: " + s);
    }

    int Z80InstructionSet::reg8Index(Reg8 r)
    {
        return static_cast<int>(r);
    }

    Z80InstructionSet::Reg16 Z80InstructionSet::parseReg16(const std::string& s)
    {
        std::string u = toUpper(s);
        if (u == "BC") return Reg16::BC;
        if (u == "DE") return Reg16::DE;
        if (u == "HL") return Reg16::HL;
        if (u == "SP") return Reg16::SP;
        if (u == "AF") return Reg16::AF;
        if (u == "IX") return Reg16::IX;
        if (u == "IY") return Reg16::IY;
        throw std::runtime_error("Unknown 16-bit register: " + s);
    }

    int Z80InstructionSet::reg16Index(Reg16 r)
    {
        return static_cast<int>(r);
    }

    bool Z80InstructionSet::isReg8(const std::string& s)
    {
        std::string u = toUpper(s);
        return u == "A" || u == "B" || u == "C" || u == "D" ||
               u == "E" || u == "H" || u == "L" || u == "F";
    }

    bool Z80InstructionSet::isReg16(const std::string& s)
    {
        std::string u = toUpper(s);
        return u == "BC" || u == "DE" || u == "HL" ||
               u == "SP" || u == "AF" || u == "IX" || u == "IY";
    }

    // ── Instruction assemblers ──────────────────────────────────────────────

    std::vector<unsigned char> Z80InstructionSet::assembleLD(const std::string& op)
    {
        if (op.find(',') != std::string::npos)
        {
            size_t comma = op.find(',');
            std::string dst = toUpper(trim(op.substr(0, comma)));
            std::string src = toUpper(trim(op.substr(comma + 1)));

            // Reg → Reg
            if (isReg8(dst) && isReg8(src))
            {
                Reg8 d = parseReg8(dst);
                Reg8 s = parseReg8(src);
                return {static_cast<unsigned char>(0x40 + (reg8Index(d) << 3) + reg8Index(s))};
            }

            // (HL) variants
            if (dst == "(HL)")
            {
                if (src == "A") return {0x71};
                int val = parseImmediate(src);
                return {0x36, static_cast<unsigned char>(val)};
            }
            if (dst == "(BC)") return {0x02};
            if (dst == "(DE)") return {0x12};

            // A, (HL) / A, (BC) / A, (DE)
            if (dst == "A")
            {
                if (src == "(HL)") return {0x7E};
                if (src == "(BC)") return {0x0A};
                if (src == "(DE)") return {0x1A};
            }

            // r, n
            if (isReg8(dst) && !isReg8(src) && src.find('(') == std::string::npos)
            {
                Reg8 d = parseReg8(dst);
                int val = parseImmediate(src);
                return {static_cast<unsigned char>(0x06 + (reg8Index(d) << 3)),
                        static_cast<unsigned char>(val)};
            }

            // 16-bit reg, n
            if (isReg16(dst) && !isReg16(src) && src.find('(') == std::string::npos)
            {
                Reg16 d = parseReg16(dst);
                int val = parseImmediate(src);
                unsigned char opcode = static_cast<unsigned char>(0x01 + (reg16Index(d) & 0x04));
                return {opcode,
                        static_cast<unsigned char>(val & 0xFF),
                        static_cast<unsigned char>((val >> 8) & 0xFF)};
            }

            throw std::runtime_error("Unsupported LD form: " + op);
        }
        else
        {
            std::string upper = toUpper(op);
            if (upper == "A,I")  return {0xED, 0x47};
            if (upper == "I,A")  return {0xED, 0x4F};
            if (upper == "A,R")  return {0xED, 0x57};
            if (upper == "R,A")  return {0xED, 0x5F};
            throw std::runtime_error("Unsupported LD form: " + op);
        }
    }

    std::vector<unsigned char> Z80InstructionSet::assembleALU(const std::string& mnem, const std::string& op)
    {
        // ADD HL, rr
        if (mnem == "ADD" && isReg16(op.substr(0, 2)) && op.find(',') != std::string::npos)
        {
            std::string upper = toUpper(op);
            if (upper == "HL,BC") return {0x09};
            if (upper == "HL,DE") return {0x19};
            if (upper == "HL,HL") return {0x29};
            if (upper == "HL,SP") return {0x39};
        }

        // ADD A, r / ADD A, n
        if (mnem == "ADD" && op.find(',') != std::string::npos)
        {
            size_t comma = op.find(',');
            std::string src = toUpper(trim(op.substr(comma + 1)));
            if (isReg8(src))
            {
                return {static_cast<unsigned char>(0x80 + reg8Index(parseReg8(src)))};
            }
            int val = parseImmediate(src);
            return {0xC6, static_cast<unsigned char>(val)};
        }

        // SUB / AND / OR / XOR
        unsigned char regBase = 0, immOpcode = 0;
        if (mnem == "SUB") { regBase = 0x90; immOpcode = 0xD6; }
        else if (mnem == "AND") { regBase = 0xA0; immOpcode = 0xE6; }
        else if (mnem == "OR")  { regBase = 0xB0; immOpcode = 0xF6; }
        else if (mnem == "XOR") { regBase = 0xA8; immOpcode = 0xEE; }

        std::string upper = toUpper(op);
        if (isReg8(upper))
        {
            return {static_cast<unsigned char>(regBase + reg8Index(parseReg8(upper)))};
        }
        int val = parseImmediate(upper);
        return {immOpcode, static_cast<unsigned char>(val)};
    }

    std::vector<unsigned char> Z80InstructionSet::assembleIncDec(const std::string& mnem, const std::string& op)
    {
        std::string upper = toUpper(op);

        if (isReg16(upper))
        {
            int idx = reg16Index(parseReg16(upper));
            unsigned char base = (mnem == "INC") ? 0x03 : 0x0B;
            return {static_cast<unsigned char>(base + (idx & 0x04))};
        }

        if (isReg8(upper))
        {
            int idx = reg8Index(parseReg8(upper));
            unsigned char base = (mnem == "INC") ? 0x04 : 0x05;
            return {static_cast<unsigned char>(base + (idx << 3))};
        }

        if (upper == "(HL)")
        {
            return {static_cast<unsigned char>((mnem == "INC") ? 0x34 : 0x35)};
        }

        throw std::runtime_error("Unsupported INC/DEC operand: " + op);
    }

    std::vector<unsigned char> Z80InstructionSet::assembleRotate(const std::string& mnem, const std::string& op)
    {
        int bit = 0;
        if (mnem == "RLC") bit = 0;
        else if (mnem == "RRC") bit = 1;
        else if (mnem == "RL")  bit = 2;
        else if (mnem == "RR")  bit = 3;
        else if (mnem == "SLA") bit = 4;
        else if (mnem == "SRA") bit = 5;
        else if (mnem == "SRL") bit = 6;

        std::string upper = toUpper(op);
        int regIdx;
        if (isReg8(upper))
        {
            regIdx = reg8Index(parseReg8(upper));
        }
        else if (upper == "(HL)")
        {
            regIdx = 7;
        }
        else
        {
            throw std::runtime_error("Unsupported rotate operand: " + op);
        }

        return {0xCB, static_cast<unsigned char>((bit << 3) + regIdx)};
    }

    std::vector<unsigned char> Z80InstructionSet::assembleBit(const std::string& mnem, const std::string& op)
    {
        size_t comma = op.find(',');
        int bitNum = parseImmediate(trim(op.substr(0, comma)));
        std::string regStr = toUpper(trim(op.substr(comma + 1)));

        int regIdx;
        if (isReg8(regStr))
        {
            regIdx = reg8Index(parseReg8(regStr));
        }
        else if (regStr == "(HL)")
        {
            regIdx = 7;
        }
        else
        {
            throw std::runtime_error("Invalid register: " + regStr);
        }

        unsigned char subOp;
        if (mnem == "BIT") subOp = static_cast<unsigned char>(0x40 + (bitNum << 3) + regIdx);
        else if (mnem == "SET") subOp = static_cast<unsigned char>(0xC0 + (bitNum << 3) + regIdx);
        else subOp = static_cast<unsigned char>(0x80 + (bitNum << 3) + regIdx);

        return {0xCB, subOp};
    }

/*
 * Utility Classes
 */



std::string Z80InstructionSet::trim(const std::string& s)
{
	size_t start = s.find_first_not_of(" \t");
	if (start == std::string::npos) return "";
	size_t end = s.find_last_not_of(" \t");
	return s.substr(start, end - start + 1);
}





std::string Z80InstructionSet::toUpper(const std::string& s)
{
	std::string r = s;
	std::transform(r.begin(), r.end(), r.begin(), ::toupper);
	return r;
}




std::string Z80InstructionSet::removeParens(const std::string& s)
{
	std::string r = s;
	r.erase(std::remove(r.begin(), r.end(), '('), r.end());
	r.erase(std::remove(r.begin(), r.end(), ')'), r.end());
	return trim(r);
}
// End of file
