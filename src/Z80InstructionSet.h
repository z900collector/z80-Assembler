#ifndef Z80_ISET_H
#define Z80_ISET_H

#include "IInstructionSet.h"

#include <string>
#include <vector>
#include <map>

class Z80InstructionSet : public IInstructionSet
{
public:
    Z80InstructionSet();

    int parseImmediate(const std::string& s) override;
    bool isLabelRef(const std::string& token) override;
    int resolveLabel(const std::string& name) override;
    void setPC(int pc) override;
    int getPC() const override;
    void advancePC(int n) override;
    void defineLabel(const std::string& name) override;
    bool hasLabel(const std::string& name) const override;
    void reset() override;
    void resetPC() override;
    int instructionSize(const std::string& mnemonic, const std::string& operand) override;
    std::vector<unsigned char>
    assemble(const std::string& mnemonic, const std::string& operand) override;

private:
    int m_pc;
    std::map<std::string, int> m_labels;

    enum class Reg8 { B, C, D, E, H, L, A, F };
    enum class Reg16 { BC, DE, HL, SP, AF, IX, IY };

    Reg8 parseReg8(const std::string& s);
    int reg8Index(Reg8 r);
    Reg16 parseReg16(const std::string& s);
    int reg16Index(Reg16 r);
    bool isReg8(const std::string& s);
    bool isReg16(const std::string& s);

    std::vector<unsigned char> assembleLD(const std::string& op);
    std::vector<unsigned char> assembleALU(const std::string& mnem, const std::string& op);
    std::vector<unsigned char> assembleIncDec(const std::string& mnem, const std::string& op);
    std::vector<unsigned char> assembleRotate(const std::string& mnem, const std::string& op);
    std::vector<unsigned char> assembleBit(const std::string& mnem, const std::string& op);

    static std::string trim(const std::string& s);
    static std::string toUpper(const std::string& s);
    static std::string removeParens(const std::string& s);
};

#endif   
