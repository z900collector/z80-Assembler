

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <map>
#include <algorithm>
#include <cctype>

// Helper to Trim whitespace from a string

std::string Trim(const std::string& str)
{
	std::cout << "Trim("<<str<<")"<<std::endl; 
	size_t first = str.find_first_not_of(" \t\r\n");
	if(first == std::string::npos)
		return "";
	size_t last = str.find_last_not_of(" \t\r\n");
	return str.substr(first, (last - first + 1));
}



// Helper to convert string to uppercase

std::string to_upper(std::string str)
{
	std::cout << "to_upper("<<str<<")"<<std::endl; 
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::toupper(c); });
	return str;
}



// Helper to parse integers (base 10 or base 16 like 0x12 or 12h)
uint32_t parse_number(std::string arg)
{
	std::cout << "parse_number("<<arg<<")"<<std::endl; 
	arg = Trim(arg);
	std::string upper_arg = to_upper(arg);
	if(upper_arg.rfind("0X", 0) == 0)
	{
		return std::stoul(arg.substr(2), nullptr, 16);
	}
	else if (arg.back() == 'h' || arg.back() == 'H')
	{
		return std::stoul(arg.substr(0, arg.length() - 1), nullptr, 16);
	}
	else
	{
		return std::stoul(arg, nullptr, 10);
	}
}





// Structure to hold dynamic instruction formats
struct InstructionFormat {
    std::string pattern; // e.g., "LD A, ", "JP ", "JR "
    uint8_t opcode;      // Base opcode byte
    bool has_imm8;       // True if it takes an 8-bit immediate
    bool has_imm16;      // True if it takes a 16-bit immediate / address
    bool is_relative;    // True if it uses a signed 8-bit PC-relative offset (like JR)
};

// Map of fixed/simple opcodes
std::map<std::string, std::vector<uint8_t>> fixed_opcodes = {
    {"NOP", {0x00}},
    {"HALT", {0x76}},
    {"RET", {0xC9}},
    {"LD A, B", {0x78}}, {"LD A, C", {0x79}}, {"LD A, D", {0x7A}}, {"LD A, E", {0x7B}},
    {"LD A, H", {0x7C}}, {"LD A, L", {0x7D}}, {"LD A, A", {0x7F}},
    {"INC A", {0x3C}}, {"DEC A", {0x3D}},
    {"INC B", {0x04}}, {"DEC B", {0x05}},
    {"INC C", {0x0C}}, {"DEC C", {0x0D}},
    {"INC D", {0x14}}, {"DEC D", {0x15}},
    {"INC E", {0x1C}}, {"DEC E", {0x1D}},
    {"INC H", {0x24}}, {"DEC H", {0x25}},
    {"ADD A, B", {0x80}}, {"SUB B", {0x90}}
};

// Simple table for variable/dynamic patterns
std::vector<InstructionFormat> patterns = {
    {"LD A, ", 0x3E, true, false, false},   // LD A, n -> 3E nn
    {"LD B, ", 0x06, true, false, false},   // LD B, n -> 06 nn
    {"LD C, ", 0x0E, true, false, false},   // LD C, n -> 0E nn
    {"JP ",    0xC3, false, true, false},   // JP nn    -> C3 ll hh
    {"JR ",    0x18, false, false, true},    // JR e     -> 18 ee (Relative)
    {"DJNZ ",    0x10, false, false, true}    // DJNZ e     -> 10 ee (Relative)
};

int main(int argc, char* argv[])
{
	if(argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << " <input_file.asm> [output_file.bin]\n";
		return 1;
	}
	std::string input_filename = argv[1];
	std::string output_filename = (argc >= 3) ? argv[2] : "output.bin";
	std::ifstream infile(input_filename);
	if (!infile.is_open())
	{
		std::cerr << "Error: Could not open input file " << input_filename << "\n";
		return 1;
	}
	std::vector<std::string> lines;
	std::string line;
	while (std::getline(infile, line))
	{
		lines.push_back(line);
	}
	infile.close();

	std::map<std::string, uint16_t> labels;
	uint16_t pc = 0x0000; // Program counter tracking addresses
	std::cout << "--- Pass 1: Resolving Labels & Origin ---\n";
	for (const auto& raw_line : lines)
	{
		std::string processed = Trim(raw_line);
		// Strip out comments
		size_t semi_pos = processed.find(';');
		if (semi_pos != std::string::npos)
		{
			processed = processed.substr(0, semi_pos);
			processed = Trim(processed);
		}
		if (processed.empty()) continue;
		std::string upper_line = to_upper(processed);

		// Check for ORG directive
		if (upper_line.rfind("ORG ", 0) == 0)
		{
			std::string arg = processed.substr(4);
			pc = static_cast<uint16_t>(parse_number(arg));
			std::cout << "Origin shifted to: 0x" << std::hex << std::setw(4) << std::setfill('0') << pc << "\n";
			continue;
		}
		// Detect and register labels
		size_t colon_pos = processed.find(':');
		if (colon_pos != std::string::npos)
		{
			std::string label_name = to_upper(Trim(processed.substr(0, colon_pos)));
			labels[label_name] = pc;
			std::cout << "Found Label: " << label_name << " at 0x" << std::hex << std::setw(4) << std::setfill('0') << pc << "\n";
			processed = Trim(processed.substr(colon_pos + 1));
			if (processed.empty()) continue;
			upper_line = to_upper(processed);
		}

		// Calculate PC steps for sizing tracking
		if (fixed_opcodes.find(upper_line) != fixed_opcodes.end())
		{
			pc += fixed_opcodes[upper_line].size();
		}
		else
		{
			bool matched = false;
			for (const auto& p : patterns)
			{
				if (upper_line.rfind(p.pattern, 0) == 0)
				{
					pc += 1; // For the base opcode byte
					if (p.has_imm8) pc += 1;
					if (p.has_imm16) pc += 2;
					if (p.is_relative) pc += 1; // Relative offset byte
					matched = true;
					break;
				}
			}
			if (!matched) pc += 3; // Fallback spacer parameter
		}
	}
	std::cout << "\n--- Pass 2: Generating Machine Code ---\n";
	std::vector<uint8_t> machine_code;
	pc = 0x0000; // Reset PC context for Pass 2 offset tracking

	for (const auto& raw_line : lines)
	{
		std::string processed = Trim(raw_line);
		size_t semi_pos = processed.find(';');
		if (semi_pos != std::string::npos)
		{
			processed = processed.substr(0, semi_pos);
			processed = Trim(processed);
		}
		size_t colon_pos = processed.find(':');
		if (colon_pos != std::string::npos)
		{
			processed = Trim(processed.substr(colon_pos + 1));
		}
		if (processed.empty()) continue;
		std::string upper_line = to_upper(processed);

		// Update tracking context for ORG
		if (upper_line.rfind("ORG ", 0) == 0)
		{
			std::string arg = processed.substr(4);
			pc = static_cast<uint16_t>(parse_number(arg));
			continue;
		}
		std::cout << std::left << std::setw(20) << processed << " -> ";
		if (fixed_opcodes.find(upper_line) != fixed_opcodes.end())
		{
			for (uint8_t b : fixed_opcodes[upper_line])
			{
				machine_code.push_back(b);
				std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)b << " ";
			}
			pc += fixed_opcodes[upper_line].size();
			std::cout << "\n";
		}
		else
		{
			bool matched = false;
			for (const auto& p : patterns)
			{
				if (upper_line.rfind(p.pattern, 0) == 0)
				{
					// 1. Output the Base Opcode
					machine_code.push_back(p.opcode);
					std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)p.opcode << " ";
					std::string arg = Trim(processed.substr(p.pattern.length()));
					std::string upper_arg = to_upper(arg);
					uint32_t val = 0;
					bool is_label = (labels.find(upper_arg) != labels.end());
					if (is_label)
					{
						val = labels[upper_arg];
					}
					else
					{
						val = parse_number(arg);
					}

					// 2. Handle standard immediate data types
					if (p.has_imm8)
					{
						uint8_t low = val & 0xFF;
						machine_code.push_back(low);
						std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)low << " ";
						pc += 2;
					}
					else if (p.has_imm16)
					{
						uint8_t low = val & 0xFF;
						uint8_t high = (val >> 8) & 0xFF;
						machine_code.push_back(low);  // Z80 multi-byte integers are Little Endian
						machine_code.push_back(high);
						std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)low << " " << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)high << " ";
						pc += 3;
					} 

					// 3. Handle Relative Offsets (JR instructions)
					else if (p.is_relative)
					{
						if (!is_label)
						{
							std::cout << "\nERROR: JR instruction requires a target label.\n";
							return 1;
						}

						// Z80 relative offset baseline calculated from target PC after executing current opcode (PC + 2)
						int32_t offset = static_cast<int32_t>(val) - (static_cast<int32_t>(pc) + 2);
						if (offset < -128 || offset > 127)
						{
							std::cout << "\nERROR: Relative jump to '" << arg << "' is out of range (" << offset << " bytes).\n";
							return 1;
						}
						uint8_t rel_byte = static_cast<uint8_t>(offset & 0xFF);
						machine_code.push_back(rel_byte);
						std::cout << "0x" << std::hex << std::setw(2) << std::setfill('0') << (int)rel_byte << " ";
						pc += 2;
					}
					std::cout << "\n";
					matched = true;
					break;
				}
			}
			if (!matched)
			{
				std::cout << "ERROR: Syntax error or unsupported pattern.\n";
				pc += 3; // Step safely over corruption fallback window
			}
		}
	}
	std::ofstream outfile(output_filename, std::ios::binary);
	return 0;
}

