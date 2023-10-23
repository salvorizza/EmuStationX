#include "RAM.h"

namespace esx {



	RAM::RAM()
		: BusDevice("RAM")
	{
		mMemory.resize(KIBI(2048));

		addRange("Root", 0x00000000, KIBI(2048), 0x1FFFFF);
		addRange("Root", 0x80000000, KIBI(2048), 0x1FFFFF);
		addRange("Root", 0xA0000000, KIBI(2048), 0x1FFFFF);
	}

	RAM::~RAM()
	{
	}


	void RAM::write(const std::string& busName, uint32_t address, uint32_t value, size_t valueSize)
	{
		for (size_t i = 0; i < valueSize; i++) {
			mMemory[address + i] = (value & 0xFF);
			value >>= 8;
		}
	}

	uint32_t RAM::read(const std::string& busName, uint32_t address, size_t outputSize)
	{
		uint32_t output = 0;
		for (size_t i = 0; i < outputSize; i++) {
			output <<= 8;
			output |= mMemory[address + (outputSize - 1 - i)];
		}
		return output;
	}

}