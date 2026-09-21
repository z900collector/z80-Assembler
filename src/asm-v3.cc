/*
 * z80 Assembler V3 using basic C++
 *
 * Compile cc file using:
 *
 * g++ -std=c++17 -Wall -Wextra -pedantic -Werror -g asm-v3.cc -o asm-v3
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
#include <stdexcept>
#include <algorithm>
#include <functional>

// ─── Abstract Instruction Set Interface ─────────────────────────────────────

class IInstructionSet
{
public:
    virtual ~IInstructionSet() = default;

    // Assemble a single instruction; returns encoded bytes.
    // Throws std::runtime_error on unknown mnemonic/operand.
    virtual std::vector<unsigned char>
    assemble(const std::string& mnemonic, const std::string& operand) = 0;

    // Parse an immediate value (hex, decimal, etc.)
    virtual int parseImmediate(const std::string& s) = 0;

    // Check whether a token is a label reference (for branch targets)
    virtual bool isLabelRef(const std::string& token) = 0;

    // Resolve a label to an absolute address
    virtual int resolveLabel(const std::string& name) = 0;

    // Set the current program counter (called by the assembler engine)
    virtual void setPC(int pc) = 0;

    // Current program counter
    virtual int getPC() const = 0;

    // Advance PC by n bytes
    virtual void advancePC(int n) = 0;

    // Register a label at the current PC
    virtual void defineLabel(const std::string& name) = 0;

    // Check if a label is defined
    virtual bool hasLabel(const std::string& name) const = 0;
};

// ─── Generic Assembler Engine ───────────────────────────────────────────────

class Assembler
{
public:
    explicit Assembler(IInstructionSet& iset)
        : m_iset(iset)
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

private:
    IInstructionSet& m_iset;
    std::string m_outputPath;
    std::vector<unsigned char> m_code;

    void processLine(const std::string& rawLine, int lineNum)
    {
        // Strip comments
        std::string line = rawLine;
        size_t commentPos = line.find(';');
        if (commentPos != std::string::npos)
        {
            line = line.substr(0, commentPos);
        }

        line = trim(line);
        if (line.empty())
        {
            return;
        }

        // Check for labels
        std::string rest = line;
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos)
        {
            std::string label = trim(line.substr(0, colonPos));
            m_iset.defineLabel(label);
            rest = trim(line.substr(colonPos + 1));
            if (rest.empty())
            {
                return;
            }
        }

        // Split mnemonic and operand
        std::string mnemonic, operand;
        size_t spacePos = rest.find_first_of(" \t");
        if (spacePos == std::string::npos)
        {
            mnemonic = toUpper(rest);
            operand = "";
        }
        else
        {
            mnemonic = toUpper(trim(rest.substr(0, spacePos)));
            operand = trim(rest.substr(spacePos + 1));
        }

        // Delegate to instruction set
        try
        {
            std::vector<unsigned char> bytes = m_iset.assemble(mnemonic, operand);
            m_code.insert(m_code.end(), bytes.begin(), bytes.end());
            m_iset.advancePC(static_cast<int>(bytes.size()));
        }
        catch (const std::runtime_error& e)
        {
            std::cerr << "Error at line " << lineNum << ": " << e.what() << "\n";
        }
    }

    void writeOutput()
    {
        std::ofstream out(m_outputPath, std::ios::binary);
        if (!out)
        {
            throw std::runtime_error("Cannot open output file: " + m_outputPath);
        }
        out.write(reinterpret_cast<const char*>(m_code.data()), m_code.size());
        out.close();

        std::cout << "Assembled " << m_code.size() << " bytes to "
                  << m_outputPath << "\n";
    }

    static std::string trim(const std::string& s)
    {
        size_t start = s.find_first_not_of(" \t");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t");
        return s.substr(start, end - start + 1);
    }

    static std::string toUpper(const std::string& s)
    {
        std::string result = s;
        std::transform(result.begin(), result.end(), result.begin(), ::toupper);
        return result;
    }
};

// ─── Z80 Instruction Set Implementation ─────────────────────────────────────

class Z80InstructionSet : public IInstructionSet
{
public:
    Z80InstructionSet()
        : m_pc(0x8000)
    {
    }

    int parseImmediate(const std::string& s) override
    {
        if (s.size() >= 2 && (s[0] == '$' ||
            (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))))
        {
            return std::stoi(s, nullptr, 16);
        }
        return std::stoi(s, nullptr, 10);
    }

    bool isLabelRef(const std::string& token) override
    {
        // A label ref is alphabetic (not a number or hex)
        if (token.empty()) return false;
        if (token[0] >= '0' && token[0] <= '9') return false;
        if (token[0] == '$') return false;
        return true;
    }

    int resolveLabel(const std::string& name) override
    {
        std::string upper = toUpper(name);
        auto it = m_labels.find(upper);
        if (it == m_labels.end())
        {
            throw std::runtime_error("Undefined label: " + upper);
        }
        return it->second;
    }

    void setPC(int pc) override { m_pc = pc; }
    int getPC() const override { return m_pc; }
    void advancePC(int n) override { m_pc += n; }

    void defineLabel(const std::string& name) override
    {
        m_labels[toUpper(name)] = m_pc;
    }

    bool hasLabel(const std::string& name) const override
    {
        return m_labels.count(toUpper(name)) > 0;
    }

    std::vector<unsigned char>
    assemble(const std::string& mnem, const std::string& op) override
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

private:
    int m_pc;
    std::map<std::string, int> m_labels;

    // ── Register helpers ────────────────────────────────────────────────────

    enum class Reg8 { B, C, D, E, H, L, A, F };
    enum class Reg16 { BC, DE, HL, SP, AF, IX, IY };

    Reg8 parseReg8(const std::string& s)
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

    int reg8Index(Reg8 r)
    {
        return static_cast<int>(r);
    }

    Reg16 parseReg16(const std::string& s)
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

    int reg16Index(Reg16 r)
    {
        return static_cast<int>(r);
    }

    bool isReg8(const std::string& s)
    {
        std::string u = toUpper(s);
        return u == "A" || u == "B" || u == "C" || u == "D" ||
               u == "E" || u == "H" || u == "L" || u == "F";
    }

    bool isReg16(const std::string& s)
    {
        std::string u = toUpper(s);
        return u == "BC" || u == "DE" || u == "HL" ||
               u == "SP" || u == "AF" || u == "IX" || u == "IY";
    }

    // ── Instruction assemblers ──────────────────────────────────────────────

    std::vector<unsigned char> assembleLD(const std::string& op)
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

    std::vector<unsigned char> assembleALU(const std::string& mnem, const std::string& op)
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

    std::vector<unsigned char> assembleIncDec(const std::string& mnem, const std::string& op)
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
            return {(mnem == "INC") ? 0x34 : 0x35};
        }

        throw std::runtime_error("Unsupported INC/DEC operand: " + op);
    }

    std::vector<unsigned char> assembleRotate(const std::string& mnem, const std::string& op)
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

    std::vector<unsigned char> assembleBit(const std::string& mnem, const std::string& op)
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

    // ── Utility ─────────────────────────────────────────────────────────────

    static std::string trim(const std::string& s)
    {
        size_t start = s.find_first_not_of(" \t");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t");
        return s.substr(start, end - start + 1);
    }

    static std::string toUpper(const std::string& s)
    {
        std::string r = s;
        std::transform(r.begin(), r.end(), r.begin(), ::toupper);
        return r;
    }

    static std::string removeParens(const std::string& s)
    {
        std::string r = s;
        r.erase(std::remove(r.begin(), r.end(), '('), r.end());
        r.erase(std::remove(r.begin(), r.end(), ')'), r.end());
        return trim(r);
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

    Z80InstructionSet z80;
    Assembler assembler(z80);
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
