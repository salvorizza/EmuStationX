#include "MemoryControl.h"

namespace esx {



	MemoryControl::MemoryControl()
		: BusDevice(ESX_TEXT("MemoryControl"))
	{
		addRange(ESX_TEXT("Root"), 0x1F801000, BYTE(36), 0xFFFFFFFF);
		addRange(ESX_TEXT("Root"), RAM_SIZE_ADDRESS, BYTE(4), 0xFFFFFFFF);
		addRange(ESX_TEXT("Root"), CACHE_CONTROL_ADDRESS, BYTE(4), 0xFFFFFFFF);

	}

	MemoryControl::~MemoryControl()
	{
	}


	void MemoryControl::store(const StringView& busName, U32 address, U32 value)
	{
		ESX_CORE_LOG_WARNING("MemoryControl - Writing to address {:08x} not implemented yet", address);
	}


}