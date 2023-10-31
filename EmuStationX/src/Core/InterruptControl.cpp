#include "InterruptControl.h"	

namespace esx {



	InterruptControl::InterruptControl()
		: BusDevice("InterruptControl")
	{
		addRange("Root", I_STAT_ADDRESS, BYTE(4), 0xFFFFFFFF);
		addRange("Root", I_MASK_ADDRESS, BYTE(4), 0xFFFFFFFF);

	}

	InterruptControl::~InterruptControl()
	{
	}

	void InterruptControl::write(const std::string& busName, uint32_t address, uint32_t value, size_t valueSize)
	{
		switch (address & (~0x1)) {
			case I_STAT_ADDRESS: {
				ESX_CORE_LOG_WARNING("Write to I_STAT not implemented yet");
				break;
			}
			case I_MASK_ADDRESS: {
				ESX_CORE_LOG_WARNING("Write to I_MASK not implemented yet");
				break;
			}
		}
	}

	uint32_t InterruptControl::read(const std::string& busName, uint32_t address, size_t outputSize)
	{
		uint32_t output = 0;

		switch (address & (~0x1)) {
			case I_STAT_ADDRESS: {
				ESX_CORE_LOG_WARNING("Reading to I_STAT not implemented yet");
				break;
			}
			case I_MASK_ADDRESS: {
				ESX_CORE_LOG_WARNING("Reading to I_MASK not implemented yet");
				break;
			}
		}

		return output;
	}

}