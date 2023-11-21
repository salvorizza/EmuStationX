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

	void InterruptControl::store(const StringView& busName, U32 address, U32 value)
	{
		switch (address) {
			case I_STAT_ADDRESS: {
				ESX_CORE_LOG_WARNING("InterruptControl - Writing to I_STAT not implemented yet");
				break;
			}
			case I_MASK_ADDRESS: {
				ESX_CORE_LOG_WARNING("InterruptControl - Writing to I_MASK not implemented yet");
				break;
			}
			default: {
				ESX_CORE_LOG_WARNING("InterruptControl - Writing to address {:08x} not implemented yet", address);
			}
		}
	}

	void InterruptControl::load(const StringView& busName, U32 address, U32& output)
	{
		output = 0;

		switch (address) {
			case I_STAT_ADDRESS: {
				ESX_CORE_LOG_WARNING("InterruptControl - Reading I_STAT not implemented yet");
				break;
			}
			case I_MASK_ADDRESS: {
				ESX_CORE_LOG_WARNING("InterruptControl - Reading I_MASK not implemented yet");
				break;
			}
			default: {
				ESX_CORE_LOG_WARNING("InterruptControl - Reading address {:08x} not implemented yet", address);
			}
		}
	}

	void InterruptControl::store(const StringView& busName, U32 address, U16 value)
	{
		ESX_CORE_LOG_WARNING("InterruptControl - Writing to address {:08x} not implemented yet", address);
	}

	void InterruptControl::load(const StringView& busName, U32 address, U16& output)
	{
		ESX_CORE_LOG_WARNING("InterruptControl - Reading address {:08x} not implemented yet", address);
	}

}