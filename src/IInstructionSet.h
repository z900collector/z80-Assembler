#ifndef IINSTRUCTIONSET_H
#define IINSTRUCTIONSET_H

#include <string>
#include <vector>

class IInstructionSet
{
public:
    virtual ~IInstructionSet() = default;

    virtual std::vector<unsigned char>
    assemble(const std::string& mnemonic, const std::string& operand) = 0;

    virtual int parseImmediate(const std::string& s) = 0;
    virtual bool isLabelRef(const std::string& token) = 0;
    virtual int resolveLabel(const std::string& name) = 0;
    virtual void setPC(int pc) = 0;
    virtual int getPC() const = 0;
    virtual void advancePC(int n) = 0;
    virtual void defineLabel(const std::string& name) = 0;
    virtual bool hasLabel(const std::string& name) const = 0;
    virtual void reset() = 0;
    virtual void resetPC() = 0;
    virtual int instructionSize(const std::string& mnemonic,
                                const std::string& operand) = 0;
};

#endif   
