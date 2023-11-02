#include "InterruptControl.h"	

namespace esx {



	InterruptControl::InterruptControl()
		: BusDevice(ESX_TEXT("InterruptControl"))
	{
		addRange(ESX_TEXT("Root"), I_STAT_ADDRESS, BYTE(4), 0xFFFFFFFF);
		addRange(ESX_TEXT("Root"), I_MASK_ADDRESS, BYTE(4), 0xFFFFFFFF);

	}

	InterruptControl::~InterruptControl()
	{
	}

	void InterruptControl::store(const String& busName, U32 address, U32 value)
	{
		ESX_CORE_LOG_WARNING("InterruptControl - Writing to address {:8x} not implemented yet", address);
	}

}