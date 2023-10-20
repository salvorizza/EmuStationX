#include "Bios.h"

namespace esx {



	Bios::Bios(const std::string& path)
		: BusDevice("Bios")
	{
		std::ifstream input(path, std::ios::binary);
		mMemory = std::vector<uint8_t>(std::istreambuf_iterator<char>(input), {});
		input.close();

		addRange("Root", 0xBFC00000, KIBI(512), 0x7FFFF);
	}

	Bios::~Bios()
	{
	}

	void Bios::write(const std::string& busName, uint32_t address, uint8_t value)
	{
		mMemory[address] = value;
	}

	void Bios::read(const std::string& busName, uint32_t address, uint8_t& output)
	{
		output = mMemory[address];
	}

	void Bios::write(const std::string& busName, uint32_t address, uint16_t value)
	{
	}

	void Bios::read(const std::string& busName, uint32_t address, uint16_t& output)
	{
		output = 0;

		for (int8_t i = (sizeof(uint16_t) - 1); i >= 0; i--) {
			output |= mMemory[address + i];
			output <<= 8;
		}
	}

	void Bios::write(const std::string& busName, uint32_t address, uint32_t value)
	{
	}

	void Bios::read(const std::string& busName, uint32_t address, uint32_t& output)
	{
		output = 0;

		for (int8_t i = (sizeof(uint32_t) - 1); i >= 0; i--) {
			output <<= 8;
			output |= mMemory[address + i];
		}
	}

}