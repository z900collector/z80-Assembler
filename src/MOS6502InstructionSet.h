#ifndef MOS6502_ISET_H
#define MOS6502_ISET_H

#include "IInstructionSet.h"
#include <string>
#include <vector>
#include <map>

class MOS6502InstructionSet : public IInstructionSet
{
public:
    MOS6502InstructionSet();

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

    std::vector<unsigned char> branch(unsigned char opcode, const std::string& op);
    std::vector<unsigned char> assembleLoadStore(const std::string& mnem, const std::string& op);
    unsigned char lookupLoadStore(int reg, bool isLoad, int mode);
    std::vector<unsigned char> assembleALU(const std::string& mnem, const std::string& op);
    std::vector<unsigned char> assembleShift(const std::string& mnem, const std::string& op);
    std::vector<unsigned char> assembleIncDec(const std::string& mnem, const std::string& op);
    static std::string trim(const std::string& s);
    static std::string toUpper(const std::string& s);
    static std::string removeParens(const std::string& s);

	static bool isHexAddr(const std::string& s);
	int parseAddr(const std::string& s);
};

#endif   
