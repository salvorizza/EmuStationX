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

	void Bios::write(const std::string& busName, uint32_t address, uint32_t value, size_t valueSize)
	{
	}

	uint32_t Bios::read(const std::string& busName, uint32_t address, size_t outputSize)
	{
		uint32_t output = 0;

		for (size_t i = 0; i < outputSize; i++) {
			output <<= 8;
			output |= mMemory[address + (outputSize - 1 - i)];
		}

		return output;
	}

}