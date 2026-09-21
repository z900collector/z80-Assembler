#include "Assembler.h"

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

Assembler::Assembler(IInstructionSet& iset): m_iset(iset)
{
}



void Assembler::setOutputFile(const std::string& path)
{
	m_outputPath = path;
}




void Assembler::assemble(const std::string& source)
{
	// Pass 1: collect all labels
	m_iset.reset();
	runPass(source, /* collectLabelsOnly */ true);
	
	// Pass 2: emit code with full label knowledge
	m_iset.resetPC();
	m_code.clear();
	runPass(source, /* collectLabelsOnly */ false);
	writeOutput();
}



void Assembler::runPass(const std::string& source, bool collectLabelsOnly)
{
	std::istringstream stream(source);
	std::string line;
	int lineNum = 0;
	while (std::getline(stream, line))
	{
		lineNum++;
		// Strip comments
		size_t commentPos = line.find(';');
		if (commentPos != std::string::npos)
		{
			line = line.substr(0, commentPos);
		}
		line = trim(line);
		if (line.empty())
		{
			continue;
		}
		// Handle labels
		std::string rest = line;
		size_t colonPos = line.find(':');
		if (colonPos != std::string::npos)
		{
			std::string label = trim(line.substr(0, colonPos));
			m_iset.defineLabel(label);
			rest = trim(line.substr(colonPos + 1));
			if (rest.empty())
			{
				continue;
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
		// Skip pseudo-ops that don't produce code (ORG)
		if (mnemonic == "ORG")
		{
			// In pass 1, we still need to set PC
			m_iset.setPC(m_iset.parseImmediate(operand));
			continue;
		}
		if (collectLabelsOnly)
		{
			// Pass 1: just advance PC by the instruction size
			int size = m_iset.instructionSize(mnemonic, operand);
			m_iset.advancePC(size);
		}
		else
		{
			// Pass 2: full assembly
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
	}
}




void Assembler::writeOutput()
{
	std::ofstream out(m_outputPath, std::ios::binary);
	if (!out)
	{
		throw std::runtime_error("Cannot open output file: " + m_outputPath);
	}
	out.write(reinterpret_cast<const char*>(m_code.data()), m_code.size());
	out.close();
	std::cout << "Assembled " << m_code.size() << " bytes to " << m_outputPath << "\n";
}



std::string Assembler::trim(const std::string& s)
{
	size_t start = s.find_first_not_of(" \t");
	if (start == std::string::npos) return "";
	size_t end = s.find_last_not_of(" \t");
	return s.substr(start, end - start + 1);
}



std::string Assembler::toUpper(const std::string& s)
{
	std::string result = s;
	std::transform(result.begin(), result.end(), result.begin(), ::toupper);
	return result;
}
// End of file
