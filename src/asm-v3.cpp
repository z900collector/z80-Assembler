/*
 * Source Code Assembler V3 using basic C++
 * Defaults to Z80 instruction set but can do 6502 using the --6502 flag
 *
 * Compile using ./build.sh
 *
 * Partly AI generated, manually edited and updated: S Young 2026
 * GITHUB Workflow added to project to do CI on push.
 *
 */

#include "IInstructionSet.h"
#include "Assembler.h"
#include "Z80InstructionSet.h"
#include "MOS6502InstructionSet.h"


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
#include <memory>

int main(int argc, char* argv[])
{
	if(argc < 2)
	{
		std::cerr << "Z80/6502 Assembler v3.0 Sid Young\n";
		std::cerr << "defaults to Z80, use --6502 to compile different target architectures.\n\n";
		std::cerr << "Usage: " << argv[0] << " <input.asm> [output.bin] [--6502]\n";
		return 1;
	}

	std::string inputFile = argv[1];
	std::string outputFile = "output.bin";
	bool is6502 = false;

	for(int i = 2; i < argc; i++)
	{
		std::string arg = argv[i];
		if(arg == "--6502")
		{
			is6502 = true;
		}
		else if(arg == "--z80")
		{
			is6502 = false;
		}
		else
		{
			outputFile = arg;
		}
	}

	std::ifstream in(inputFile);
	if(!in)
	{
		std::cerr << "Cannot open input file: " << inputFile << "\n";
		return 1;
	}

	std::stringstream buffer;
	buffer << in.rdbuf();
	std::string source = buffer.str();

	// Select instruction set
	std::unique_ptr<IInstructionSet> iset;
	if(is6502)
	{
		iset = std::make_unique<MOS6502InstructionSet>();
	}
	else
	{
		iset = std::make_unique<Z80InstructionSet>();
	}

	Assembler assembler(*iset);
	assembler.setOutputFile(outputFile);
	try
	{
		assembler.assemble(source);
	}
	catch(const std::runtime_error& e)
	{
		std::cerr << "Assembly error: " << e.what() << "\n";
		return 1;
	}
	return 0;
}
// End of file
