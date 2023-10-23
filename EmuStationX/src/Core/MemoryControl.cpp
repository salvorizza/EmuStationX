#include "MemoryControl.h"

namespace esx {



	MemoryControl::MemoryControl()
		: BusDevice("MemoryControl")
	{
		addRange("Root", 0x1F801000, BYTE(36), 0xFFFFFFFF);
		addRange("Root", RAM_SIZE_ADDRESS, BYTE(4), 0xFFFFFFFF);
		addRange("Root", CACHE_CONTROL_ADDRESS, BYTE(4), 0xFFFFFFFF);

	}

	MemoryControl::~MemoryControl()
	{
	}

	void MemoryControl::write(const std::string& busName, uint32_t address, uint32_t value, size_t valueSize)
	{
		switch (address) {
			case EXP1_BASE_ADDR_ADDRESS: {
				mIORegisters.EXP1_BASE_ADDR = value;
				break;
			}
			case EXP2_BASE_ADDR_ADDRESS: {
				mIORegisters.EXP2_BASE_ADDR = value;
				break;
			}
			case EXP1_DELAY_ADDRESS: {
				mIORegisters.EXP1_DELAY = value;
				break;
			}
			case EXP3_DELAY_ADDRESS: {
				mIORegisters.EXP3_DELAY = value;
				break;
			}
			case BIOS_ROM_DELAY_ADDRESS: {
				mIORegisters.BIOS_ROM_DELAY = value;
				break;
			}
			case SPU_DELAY_ADDRESS: {
				mIORegisters.SPU_DELAY = value;
				break;
			}
			case CDROM_DELAY_ADDRESS: {
				mIORegisters.CDROM_DELAY = value;
				break;
			}
			case EXP2_DELAY_ADDRESS: {
				mIORegisters.EXP2_DELAY = value;
				break;
			}
			case COM_DELAY_ADDRESS: {
				mIORegisters.COM_DELAY = value;
				break;
			}
			case RAM_SIZE_ADDRESS: {
				mIORegisters.RAM_SIZE = value;
				break;
			}
			case CACHE_CONTROL_ADDRESS: {
				mIORegisters.CACHE_CONTROL = value;
				break;
			}
		}
	}

	uint32_t MemoryControl::read(const std::string& busName, uint32_t address, size_t outputSize)
	{
		uint32_t output = 0;

		switch (address) {
			case EXP1_BASE_ADDR_ADDRESS: {
				output = mIORegisters.EXP1_BASE_ADDR;
				break;
			}
			case EXP2_BASE_ADDR_ADDRESS: {
				output = mIORegisters.EXP2_BASE_ADDR;
				break;
			}
			case EXP1_DELAY_ADDRESS: {
				output = mIORegisters.EXP1_DELAY;
				break;
			}
			case EXP3_DELAY_ADDRESS: {
				output = mIORegisters.EXP3_DELAY;
				break;
			}
			case BIOS_ROM_DELAY_ADDRESS: {
				output = mIORegisters.BIOS_ROM_DELAY;
				break;
			}
			case SPU_DELAY_ADDRESS: {
				output = mIORegisters.SPU_DELAY;
				break;
			}
			case CDROM_DELAY_ADDRESS: {
				output = mIORegisters.CDROM_DELAY;
				break;
			}
			case EXP2_DELAY_ADDRESS: {
				output = mIORegisters.EXP2_DELAY;
				break;
			}
			case COM_DELAY_ADDRESS: {
				output = mIORegisters.COM_DELAY;
				break;
			}
			case RAM_SIZE_ADDRESS: {
				output = mIORegisters.RAM_SIZE;
				break;
			}
			case CACHE_CONTROL_ADDRESS: {
				output = mIORegisters.CACHE_CONTROL;
				break;
			}
		}

		return output;
	}

}