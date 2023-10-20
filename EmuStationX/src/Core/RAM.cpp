#include "RAM.h"

namespace esx {



	RAM::RAM()
		: BusDevice("RAM")
	{
		mMemory.resize(KIBI(2048));

		constexpr int p = 2048 * 1024;

		addRange("Root", 0xA0000000, KIBI(2048), 0x1FFFFF);
	}

	RAM::~RAM()
	{
	}

	void RAM::write(const std::string& busName, uint32_t address, uint8_t value)
	{
		mMemory[address] = value;
	}

	void RAM::read(const std::string& busName, uint32_t address, uint8_t& output)
	{
		output = mMemory[address];
	}

	void RAM::write(const std::string& busName, uint32_t address, uint16_t value)
	{
	}

	void RAM::read(const std::string& busName, uint32_t address, uint16_t& output)
	{
		for (int8_t i = (sizeof(uint16_t) - 1); i >= 0; i--) {
			output <<= 8;
			output |= mMemory[address + i];
		}
	}

	void RAM::write(const std::string& busName, uint32_t address, uint32_t value)
	{
	}

	void RAM::read(const std::string& busName, uint32_t address, uint32_t& output)
	{
		for (int8_t i = (sizeof(uint32_t) - 1); i >= 0; i--) {
			output <<= 8;
			output |= mMemory[address + i];
		}
	}

}