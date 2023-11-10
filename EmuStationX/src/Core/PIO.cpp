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
		if (address == 0x1F802041) {
			ESX_CORE_LOG_TRACE("PIO - POST 0x{:02X}", value);
		} else {
			ESX_CORE_LOG_WARNING("PIO - Writing to address {:8x} not implemented yet", address);
		}
	}

	void PIO::load(const String& busName, U32 address, U8& output)
	{
		ESX_CORE_LOG_WARNING("PIO - Reading from address {:8x} not implemented yet", address);
	}

}