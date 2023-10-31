#include "PIO.h"

#include <iostream>

namespace esx {

	PIO::PIO()
		: BusDevice("PIO")
	{
		addRange("Root", 0x1F000000, KIBI(512), 0xFFFFFFFF);
		addRange("Root", 0x1F802000, BYTE(133), 0xFFFFFFFF);
		addRange("Root", 0x1FA00000, BYTE(1), 0xFFFFFFFF);
	}

	PIO::~PIO()
	{
	}

	void PIO::write(const std::string& busName, uint32_t address, uint32_t value, size_t valueSize)
	{
		ESX_CORE_LOG_WARNING("Writing to PIO Address 0x{:8X} not handled yet", address);
	}

	uint32_t PIO::read(const std::string& busName, uint32_t address, size_t outputSize)
	{
		uint32_t output = 0;

		ESX_CORE_LOG_WARNING("Reading to PIO Address 0x{:8X} not handled yet", address);

		return output;
	}

}