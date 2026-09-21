/*
 * z80 Assembler using very basic C++
 *
 * Compile cc file using:
 *
 * g++ -std=c++17 -Wall -Wextra -pedantic -Werror -g asm-v2.cc -o asm-v2
 *
 * Partly AI generated, manually edited and updated: S Young 2026
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <cctype>
#include <cstdlib>
#include <algorithm>
#include <stdexcept>


enum class Reg8
{
    B, C, D, E, H, L, A, F
};

enum class Reg16
{
    BC, DE, HL, SP, AF, IX, IY
};


// Hold the instruction and operands
struct Instruction
{
    std::vector<unsigned char> bytes;
    int size;
};




/*========================================
 *
 *                 Helpers
 *
 *========================================
 */

int ParseHex(const std::string& s)
{
    return std::stoi(s, nullptr, 16);
}



/*========================================
 *
 * $DD $DDDD
 * 0xDDDD  0XDDDD
 *========================================
 */
int parseImmediate(const std::string& s)
{
	if (s.size() >= 2 && (s[0] == '$' || (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))))
	{
		return std::stoi(s, nullptr, 16);
	}
	if (s == "0")
	{
		return 0;
	}
	return std::stoi(s, nullptr, 10);
}



/*========================================
 *
 * convert to uppercase and compare,
 * then return enum
 *========================================
 */

Reg8 parseReg8(const std::string& s)
{
    std::string upper = s;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    if (upper == "B") return Reg8::B;
    if (upper == "C") return Reg8::C;
    if (upper == "D") return Reg8::D;
    if (upper == "E") return Reg8::E;
    if (upper == "H") return Reg8::H;
    if (upper == "L") return Reg8::L;
    if (upper == "A") return Reg8::A;
    if (upper == "F") return Reg8::F;
    throw std::runtime_error("Unknown 8-bit register: " + s);
}



int reg8Index(Reg8 r)
{
	switch (r)
	{
		case Reg8::B: return 0;
		case Reg8::C: return 1;
		case Reg8::D: return 2;
		case Reg8::E: return 3;
		case Reg8::H: return 4;
		case Reg8::L: return 5;
		case Reg8::A: return 6;
		case Reg8::F: return 7;
	}
	return 0;
}




Reg16 parseReg16(const std::string& s)
{
    std::string upper = s;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    if (upper == "BC") return Reg16::BC;
    if (upper == "DE") return Reg16::DE;
    if (upper == "HL") return Reg16::HL;
    if (upper == "SP") return Reg16::SP;
    if (upper == "AF") return Reg16::AF;
    if (upper == "IX") return Reg16::IX;
    if (upper == "IY") return Reg16::IY;
    throw std::runtime_error("Unknown 16-bit register: " + s);
}

int reg16Index(Reg16 r)
{
    switch (r)
    {
        case Reg16::BC: return 0;
        case Reg16::DE: return 1;
        case Reg16::HL: return 2;
        case Reg16::SP: return 3;
        case Reg16::AF: return 4;
        case Reg16::IX: return 5;
        case Reg16::IY: return 6;
    }
    return 0;
}

std::string trim(const std::string& s)
{
    size_t start = s.find_first_not_of(" \t");
    if (start == std::string::npos)
    {
        return "";
    }
    size_t end = s.find_last_not_of(" \t");
    return s.substr(start, end - start + 1);
}

std::string toUpper(const std::string& s)
{
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

// ─── Assembler ───────────────────────────────────────────────────────────────

class Z80Assembler
{
public:
    Z80Assembler()
        : m_org(0x8000)
    {
    }

    void setOutputFile(const std::string& path)
    {
        m_outputPath = path;
    }

    void assemble(const std::string& source)
    {
        std::istringstream stream(source);
        std::string line;
        int lineNum = 0;

        while (std::getline(stream, line))
        {
            lineNum++;
            processLine(line, lineNum);
        }

        writeOutput();
    }

    void processLine(const std::string& rawLine, int lineNum)
    {
        // Strip comments
        std::string line = rawLine;
        size_t commentPos = line.find(';');
        if (commentPos != std::string::npos)
        {
            line = line.substr(0, commentPos);
	std::cout<<"Line: "<<line<<std::endl;
        }

        line = trim(line);
        if (line.empty())
        {
            return;
        }

        // Check for labels
        std::string mnemonic, operand;
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos)
        {
            std::string label = trim(line.substr(0, colonPos));
            std::string rest = trim(line.substr(colonPos + 1));
            m_labels[toUpper(label)] = m_pc;
            if (rest.empty())
            {
                return;
            }
            line = rest;
        }

        // Split mnemonic and operand
        size_t spacePos = line.find_first_of(" \t");
        if (spacePos == std::string::npos)
        {
            mnemonic = toUpper(line);
            operand = "";
        }
        else
        {
            mnemonic = toUpper(trim(line.substr(0, spacePos)));
            operand = trim(line.substr(spacePos + 1));
        }

        try
        {
            assembleInstruction(mnemonic, operand);
        }
        catch (const std::runtime_error& e)
        {
            std::cerr << "Error at line " << lineNum << ": " << e.what() << "\n";
        }
    }

/*========================================
 *
 *
 *
 *========================================
 */
 void assembleInstruction(const std::string& mnem, const std::string& op)
 {
	if (mnem == "ORG")
        {
	            m_org = parseImmediate(op);
            m_pc = m_org;
            return;
        }

        if (mnem == "DB")
        {
            std::istringstream iss(op);
            std::string token;
            while (iss >> token)
            {
                m_code.push_back(static_cast<unsigned char>(parseImmediate(token)));
                m_pc++;
            }
            return;
        }

        if (mnem == "DW")
        {
            std::istringstream iss(op);
            std::string token;
            while (iss >> token)
            {
                int val = parseImmediate(token);
                m_code.push_back(static_cast<unsigned char>(val & 0xFF));
                m_code.push_back(static_cast<unsigned char>((val >> 8) & 0xFF));
                m_pc += 2;
            }
            return;
        }

        if (mnem == "DS")
        {
            int count = parseImmediate(op);
            for (int i = 0; i < count; i++)
            {
                m_code.push_back(0x00);
                m_pc++;
            }
            return;
        }

        // ── 1-byte instructions ─────────────────────────────────────────────
        if (mnem == "NOP")
        {
            emit(0x00);
            return;
        }
        if (mnem == "HALT")
        {
            emit(0x76);
            return;
        }
        if (mnem == "RET")
        {
            emit(0xC9);
            return;
        }
        if (mnem == "RETN")
        {
            emit(0xC5);
            return;
        }
        if (mnem == "RETI")
        {
            emit(0x45);
            return;
        }
        if (mnem == "DAA")
        {
            emit(0x27);
            return;
        }
        if (mnem == "CPL")
        {
            emit(0x2F);
            return;
        }
        if (mnem == "CCF")
        {
            emit(0x3F);
            return;
        }
        if (mnem == "SCF")
        {
            emit(0x37);
            return;
        }
        if (mnem == "DI")
        {
            emit(0xF3);
            return;
        }
        if (mnem == "EI")
        {
            emit(0xFB);
            return;
        }
        if (mnem == "EXAF")
        {
            emit(0x08);
            return;
        }
        if (mnem == "EXX")
        {
            emit(0xD9);
            return;
        }
        if (mnem == "IM")
        {
            int mode = parseImmediate(op);
            emit(0xED);
            emit(static_cast<unsigned char>(0x46 + mode));
            return;
        }

        // ── RST ─────────────────────────────────────────────────────────────
        if (mnem == "RST")
        {
            int addr = parseImmediate(op);
            emit(static_cast<unsigned char>(0xC7 + (addr >> 3)));
            return;
        }

        // ── JP / CALL / RET cc ──────────────────────────────────────────────
        if (mnem == "JP")
        {
            emitJumpOrCall(0xC3, op);
            return;
        }
        if (mnem == "JR")
        {
            int offset = resolveLabel(op);
            emit(0x18);
            emit(static_cast<unsigned char>(static_cast<signed char>(offset)));
            return;
        }
        if (mnem == "CALL")
        {
            emitJumpOrCall(0xCD, op);
            return;
        }
        if (mnem == "DJNZ")
        {
            int offset = resolveLabel(op);
            emit(0x10);
            emit(static_cast<unsigned char>(static_cast<signed char>(offset)));
            return;
        }

        // ── PUSH / POP ──────────────────────────────────────────────────────
        if (mnem == "PUSH" || mnem == "POP")
        {
            Reg16 r = parseReg16(op);
            int idx = reg16Index(r);
            if (mnem == "PUSH")
            {
                emit(static_cast<unsigned char>(0xC5 + (idx & 0x04)));
            }
            else
            {
                emit(static_cast<unsigned char>(0xF5 + (idx & 0x04)));
            }
            return;
        }

        // ── EX (DE,HL) / EX (SP,HL) ────────────────────────────────────────
        if (mnem == "EX")
        {
            std::string clean = op;
            // Remove parentheses
            clean.erase(std::remove(clean.begin(), clean.end(), '('), clean.end());
            clean.erase(std::remove(clean.begin(), clean.end(), ')'), clean.end());
            clean = trim(clean);
            if (toUpper(clean) == "DE,HL")
            {
                emit(0xEB);
            }
            else if (toUpper(clean) == "SP,HL")
            {
                emit(0xE3);
            }
            else
            {
                throw std::runtime_error("Unknown EX operand: " + op);
            }
            return;
        }

        // ── IN / OUT ────────────────────────────────────────────────────────
        if (mnem == "IN")
        {
            // IN A,(n) or IN (C)
            if (op.find(',') != std::string::npos)
            {
                emit(0xD8); // IN (C)
            }
            else
            {
                int port = parseImmediate(op);
                emit(0xDB);
                emit(static_cast<unsigned char>(port));
            }
            return;
        }
        if (mnem == "OUT")
        {
            if (op.find(',') != std::string::npos)
            {
                emit(0xC8); // OUT (C),A
            }
            else
            {
                int port = parseImmediate(op);
                emit(0xD3);
                emit(static_cast<unsigned char>(port));
            }
            return;
        }

        // ── LD variants ─────────────────────────────────────────────────────
        if (mnem == "LD")
        {
            assembleLD(op);
            return;
        }

        // ── ALU: ADD, SUB, AND, OR, XOR ─────────────────────────────────────
        if (mnem == "ADD" || mnem == "SUB" || mnem == "AND" || mnem == "OR" || mnem == "XOR")
        {
            assembleALU(mnem, op);
            return;
        }

        // ── INC / DEC (8-bit) ───────────────────────────────────────────────
        if (mnem == "INC" || mnem == "DEC")
        {
            assembleIncDec(mnem, op);
            return;
        }

        // ── Rotate / Shift: RLC, RRC, RL, RR, SLA, SRA, SRL ─────────────────
        if (mnem == "RLC" || mnem == "RRC" || mnem == "RL" || mnem == "RR" ||
            mnem == "SLA" || mnem == "SRA" || mnem == "SRL")
        {
            assembleRotate(mnem, op);
            return;
        }

        // ── BIT / SET / RES ─────────────────────────────────────────────────
        if (mnem == "BIT" || mnem == "SET" || mnem == "RES")
        {
            assembleBit(mnem, op);
            return;
        }

        throw std::runtime_error("Unknown mnemonic: " + mnem);
    }

private:
    int m_org;
    int m_pc;
    std::string m_outputPath;
    std::vector<unsigned char> m_code;
    std::map<std::string, int> m_labels;

    void emit(unsigned char byte)
    {
        m_code.push_back(byte);
        m_pc++;
    }

    void emit16(int value)
    {
        emit(static_cast<unsigned char>(value & 0xFF));
        emit(static_cast<unsigned char>((value >> 8) & 0xFF));
    }

    int resolveLabel(const std::string& op)
    {
        std::string label = toUpper(trim(op));
        auto it = m_labels.find(label);
        if (it == m_labels.end())
        {
            throw std::runtime_error("Undefined label: " + label);
        }
        return it->second - (m_pc + 2); // relative to next instruction
    }

    void emitJumpOrCall(unsigned char prefix, const std::string& op)
    {
        std::string label = toUpper(trim(op));
        auto it = m_labels.find(label);
        if (it == m_labels.end())
        {
            throw std::runtime_error("Undefined label: " + label);
        }
        emit(prefix);
        emit16(it->second);
    }

    void assembleLD(const std::string& op)
    {
        // LD A,B / LD B,A / LD A,n / LD n,A
        if (op.find(',') != std::string::npos)
        {
            size_t comma = op.find(',');
            std::string dst = toUpper(trim(op.substr(0, comma)));
            std::string src = toUpper(trim(op.substr(comma + 1)));

            // Register to Register
            if (isReg8Name(dst) && isReg8Name(src))
            {
                Reg8 d = parseReg8(dst);
                Reg8 s = parseReg8(src);
                emit(static_cast<unsigned char>(0x40 + (reg8Index(d) << 3) + reg8Index(s)));
                return;
            }

            // 16-bit to 16-bit
            if (isReg16Name(dst) && isReg16Name(src))
            {
                Reg16 d = parseReg16(dst);
                Reg16 s = parseReg16(src);
                int di = reg16Index(d);
                int si = reg16Index(s);
                emit(static_cast<unsigned char>(0x01 + (di & 0x04)));
                emit(static_cast<unsigned char>(0x00 + (si & 0x07) * 0x10));
                // This is a simplification; proper 16-bit LD needs a table
                return;
            }

            // LD (HL), n
            if (dst == "(HL)")
            {
                int val = parseImmediate(src);
                emit(0x36);
                emit(static_cast<unsigned char>(val));
                return;
            }

            // LD (BC), A / LD (DE), A
            if (dst == "(BC)")
            {
                emit(0x02);
                return;
            }
            if (dst == "(DE)")
            {
                emit(0x12);
                return;
            }

            // LD A, (HL)
            if (dst == "A" && src == "(HL)")
            {
                emit(0x7E);
                return;
            }

            // LD A, (BC)
            if (dst == "A" && src == "(BC)")
            {
                emit(0x0A);
                return;
            }

            // LD A, (DE)
            if (dst == "A" && src == "(DE)")
            {
                emit(0x1A);
                return;
            }

            // LD A, (nn)
            if (dst == "A" && isAddr(src))
            {
                int addr = parseImmediate(src);
                emit(0x3A);
                emit16(addr);
                return;
            }

            // LD (nn), A
            if (isAddr(dst) && src == "A")
            {
                int addr = parseImmediate(dst);
                emit(0x32);
                emit16(addr);
                return;
            }

            // LD r, n
            if (isReg8Name(dst) && !isReg8Name(src) && src != "(HL)")
            {
                Reg8 d = parseReg8(dst);
                int val = parseImmediate(src);
                emit(static_cast<unsigned char>(0x06 + (reg8Index(d) << 3)));
                emit(static_cast<unsigned char>(val));
                return;
            }

            // LD HL, nn
            if (dst == "HL" && !isReg16Name(src) && src.find(',') == std::string::npos)
            {
                int val = parseImmediate(src);
                emit(0x21);
                emit16(val);
                return;
            }

            // LD BC, nn / LD DE, nn / LD SP, nn
            if (isReg16Name(dst) && !isReg16Name(src) && src.find(',') == std::string::npos)
            {
                Reg16 d = parseReg16(dst);
                int val = parseImmediate(src);
                int di = reg16Index(d);
                emit(static_cast<unsigned char>(0x01 + (di & 0x04)));
                emit16(val);
                return;
            }

            throw std::runtime_error("Unsupported LD form: " + op);
        }
        else
        {
            // LD A, I / LD A, R / LD I, A / LD R, A
            std::string upper = toUpper(op);
            if (upper == "A,I")
            {
                emit(0xED);
                emit(0x47);
                return;
            }
            if (upper == "I,A")
            {
                emit(0xED);
                emit(0x4F);
                return;
            }
            if (upper == "A,R")
            {
                emit(0xED);
                emit(0x57);
                return;
            }
            if (upper == "R,A")
            {
                emit(0xED);
                emit(0x5F);
                return;
            }
            throw std::runtime_error("Unsupported LD form: " + op);
        }
    }

    void assembleALU(const std::string& mnem, const std::string& op)
    {
        // ADD A, n / SUB n / AND n / OR n / XOR n
        if (mnem == "ADD" && op.find(',') != std::string::npos)
        {
            size_t comma = op.find(',');
            std::string dst = toUpper(trim(op.substr(0, comma)));
            std::string src = toUpper(trim(op.substr(comma + 1)));

            if (dst == "A" && isReg8Name(src))
            {
                Reg8 s = parseReg8(src);
                emit(static_cast<unsigned char>(0x80 + reg8Index(s)));
                return;
            }
            if (dst == "A" && !isReg8Name(src))
            {
                int val = parseImmediate(src);
                emit(0xC6);
                emit(static_cast<unsigned char>(val));
                return;
            }
        }

        // ADD HL, rr
	if (mnem == "ADD" && (toUpper(op) == "HL,BC" || toUpper(op) == "HL,DE" ||
	    toUpper(op) == "HL,HL" || toUpper(op) == "HL,SP"))   
        {
            std::string upper = toUpper(op);
            if (upper == "HL,BC")
            {
                emit(0x09);
            }
            else if (upper == "HL,DE")
            {
                emit(0x19);
            }
            else if (upper == "HL,HL")
            {
                emit(0x29);
            }
            else
            {
                emit(0x39);
            }
            return;
        }

        // SUB n / AND n / OR n / XOR n (immediate)
        if (mnem == "SUB")
        {
            if (isReg8Name(op))
            {
                Reg8 s = parseReg8(op);
                emit(static_cast<unsigned char>(0x90 + reg8Index(s)));
            }
            else
            {
                int val = parseImmediate(op);
                emit(0xD6);
                emit(static_cast<unsigned char>(val));
            }
            return;
        }

        if (mnem == "AND")
        {
            if (isReg8Name(op))
            {
                Reg8 s = parseReg8(op);
                emit(static_cast<unsigned char>(0xA0 + reg8Index(s)));
            }
            else
            {
                int val = parseImmediate(op);
                emit(0xE6);
                emit(static_cast<unsigned char>(val));
            }
            return;
        }

        if (mnem == "OR")
        {
            if (isReg8Name(op))
            {
                Reg8 s = parseReg8(op);
                emit(static_cast<unsigned char>(0xB0 + reg8Index(s)));
            }
            else
            {
                int val = parseImmediate(op);
                emit(0xF6);
                emit(static_cast<unsigned char>(val));
            }
            return;
        }

        if (mnem == "XOR")
        {
            if (isReg8Name(op))
            {
                Reg8 s = parseReg8(op);
                emit(static_cast<unsigned char>(0xA8 + reg8Index(s)));
            }
            else
            {
                int val = parseImmediate(op);
                emit(0xEE);
                emit(static_cast<unsigned char>(val));
            }
            return;
        }

        throw std::runtime_error("Unsupported ALU form: " + mnem + " " + op);
    }

    void assembleIncDec(const std::string& mnem, const std::string& op)
    {
        std::string upper = toUpper(op);

        // 16-bit INC/DEC
        if (isReg16Name(upper))
        {
            Reg16 r = parseReg16(upper);
            int idx = reg16Index(r);
            if (mnem == "INC")
            {
                emit(static_cast<unsigned char>(0x03 + (idx & 0x04)));
            }
            else
            {
                emit(static_cast<unsigned char>(0x0B + (idx & 0x04)));
            }
            return;
        }

        // 8-bit INC/DEC
        if (isReg8Name(upper))
        {
            Reg8 r = parseReg8(upper);
            int idx = reg8Index(r);
            if (mnem == "INC")
            {
                emit(static_cast<unsigned char>(0x04 + (idx << 3)));
            }
            else
            {
                emit(static_cast<unsigned char>(0x05 + (idx << 3)));
            }
            return;
        }

        // INC (HL) / DEC (HL)
        if (upper == "(HL)")
        {
            if (mnem == "INC")
            {
                emit(0x34);
            }
            else
            {
                emit(0x35);
            }
            return;
        }

        throw std::runtime_error("Unsupported INC/DEC operand: " + op);
    }

    void assembleRotate(const std::string& mnem, const std::string& op)
    {
        std::string upper = toUpper(op);
        int bit = 0; // 0=RLC, 1=RRC, 2=RL, 3=RR, 4=SLA, 5=SRA, 6=SRL
        if (mnem == "RLC") bit = 0;
        else if (mnem == "RRC") bit = 1;
        else if (mnem == "RL") bit = 2;
        else if (mnem == "RR") bit = 3;
        else if (mnem == "SLA") bit = 4;
        else if (mnem == "SRA") bit = 5;
        else if (mnem == "SRL") bit = 6;

        if (isReg8Name(upper))
        {
            Reg8 r = parseReg8(upper);
            emit(0xCB);
            emit(static_cast<unsigned char>(0x00 + (bit << 3) + reg8Index(r)));
            return;
        }

        if (upper == "(HL)")
        {
            emit(0xCB);
            emit(static_cast<unsigned char>(0x00 + (bit << 3) + 7));
            return;
        }

        throw std::runtime_error("Unsupported rotate operand: " + op);
    }

    void assembleBit(const std::string& mnem, const std::string& op)
    {
        size_t comma = op.find(',');
        if (comma == std::string::npos)
        {
            throw std::runtime_error("Expected bit,register form: " + op);
        }

        int bitNum = parseImmediate(trim(op.substr(0, comma)));
        std::string regStr = toUpper(trim(op.substr(comma + 1)));

        int regIdx;
        if (isReg8Name(regStr))
        {
            regIdx = reg8Index(parseReg8(regStr));
        }
        else if (regStr == "(HL)")
        {
            regIdx = 7;
        }
        else
        {
            throw std::runtime_error("Invalid register for BIT/SET/RES: " + regStr);
        }

        unsigned char cbOp;
        if (mnem == "BIT")
        {
            cbOp = static_cast<unsigned char>(0x40 + (bitNum << 3) + regIdx);
        }
        else if (mnem == "SET")
        {
            cbOp = static_cast<unsigned char>(0xC0 + (bitNum << 3) + regIdx);
        }
        else
        {
            cbOp = static_cast<unsigned char>(0x80 + (bitNum << 3) + regIdx);
        }

        emit(0xCB);
        emit(cbOp);
    }

    bool isReg8Name(const std::string& s)
    {
        return s == "A" || s == "B" || s == "C" || s == "D" ||
               s == "E" || s == "H" || s == "L" || s == "F";
    }

    bool isReg16Name(const std::string& s)
    {
        return s == "BC" || s == "DE" || s == "HL" ||
               s == "SP" || s == "AF" || s == "IX" || s == "IY";
    }

    bool isAddr(const std::string& s)
    {
        return (s.size() >= 2 && s[0] == '$') ||
               (s.size() >= 3 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'));
    }

    void writeOutput()
    {
        // Pad from org to start of code
        std::vector<unsigned char> output;
        int startAddr = m_org;
        if (m_pc < startAddr)
        {
            throw std::runtime_error("Code size exceeds available space");
        }

        for (int i = 0; i < static_cast<int>(m_code.size()); i++)
        {
            output.push_back(m_code[i]);
        }

        std::ofstream out(m_outputPath, std::ios::binary);
        if (!out)
        {
            throw std::runtime_error("Cannot open output file: " + m_outputPath);
        }
        out.write(reinterpret_cast<const char*>(output.data()), output.size());
        out.close();

        std::cout << "Assembled " << output.size() << " bytes to "
                  << m_outputPath << "\n";
    }
};

// ─── Main ────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <input.asm> [output.bin]\n";
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = (argc >= 3) ? argv[2] : "output.bin";

    std::ifstream in(inputFile);
    if (!in)
    {
        std::cerr << "Cannot open input file: " << inputFile << "\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string source = buffer.str();

    Z80Assembler assembler;
    assembler.setOutputFile(outputFile);

    try
    {
        assembler.assemble(source);
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << "Assembly error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}   
