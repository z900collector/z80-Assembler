#ifndef ASSEMBLER_HDR
#define ASSEMBLER_HDR

#include <string>
#include <vector>

#include "IInstructionSet.h"

class Assembler
{
public:
    explicit Assembler(IInstructionSet& iset);

    void setOutputFile(const std::string& path);
    void assemble(const std::string& source);

private:
    IInstructionSet& m_iset;
    std::string m_outputPath;
    std::vector<unsigned char> m_code;

    void runPass(const std::string& source, bool collectLabelsOnly);
    void writeOutput();

    static std::string trim(const std::string& s);
    static std::string toUpper(const std::string& s);
};

#endif   
