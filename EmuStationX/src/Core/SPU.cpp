#include "SPU.h"

namespace esx {



	SPU::SPU()
		: BusDevice(ESX_TEXT("SPU"))
	{
		addRange(ESX_TEXT("Root"), 0x1F801C00, BYTE(640), 0xFFFFFFFF);
		
	}

	SPU::~SPU()
	{
	}

	void SPU::store(const String& busName, U32 address, U16 value)
	{
		//ESX_CORE_LOG_WARNING("SPU - Writing to address {:8x} not implemented yet", address);
	}

	void SPU::load(const String& busName, U32 address, U16& output)
	{
		//ESX_CORE_LOG_WARNING("SPU - Reading from address {:8x} not implemented yet", address);
	}

}