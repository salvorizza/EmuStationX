#include "PIO.h"

#include <iostream>

namespace esx {

	PIO::PIO()
		: BusDevice(ESX_TEXT("PIO"))
	{
		addRange(ESX_TEXT("Root"), 0x1F000000, KIBI(512), 0xFFFFFFFF);
		addRange(ESX_TEXT("Root"), 0x1F802000, BYTE(133), 0xFFFFFFFF);
		addRange(ESX_TEXT("Root"), 0x1FA00000, BYTE(1), 0xFFFFFFFF);
	}

	PIO::~PIO()
	{
	}

	void PIO::store(const String& busName, U32 address, U8 value)
	{
		ESX_CORE_LOG_WARNING("SPU - Writing to address {:8x} not implemented yet", address);
	}

	void PIO::load(const String& busName, U32 address, U8& output)
	{
		ESX_CORE_LOG_WARNING("SPU - Reading from address {:8x} not implemented yet", address);
	}

}