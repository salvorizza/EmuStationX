#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {

	constexpr size_t EXP1_BASE_ADDR_ADDRESS = 0x1F801000;
	constexpr size_t EXP2_BASE_ADDR_ADDRESS = 0x1F801004;
	constexpr size_t EXP1_DELAY_ADDRESS = 0x1F801008;
	constexpr size_t EXP3_DELAY_ADDRESS = 0x1F80100C;
	constexpr size_t BIOS_ROM_DELAY_ADDRESS = 0x1F801010;
	constexpr size_t SPU_DELAY_ADDRESS = 0x1F801014;
	constexpr size_t CDROM_DELAY_ADDRESS = 0x1F801018;
	constexpr size_t EXP2_DELAY_ADDRESS = 0x1F80101C;
	constexpr size_t COM_DELAY_ADDRESS = 0x1F801020;
	constexpr size_t RAM_SIZE_ADDRESS = 0x1F801060;
	constexpr size_t CACHE_CONTROL_ADDRESS = 0xFFFE0130;

	struct IORegisters {
		uint32_t EXP1_BASE_ADDR;
		uint32_t EXP2_BASE_ADDR;
		uint32_t EXP1_DELAY;
		uint32_t EXP3_DELAY;
		uint32_t BIOS_ROM_DELAY;
		uint32_t SPU_DELAY;
		uint32_t CDROM_DELAY;
		uint32_t EXP2_DELAY;
		uint32_t COM_DELAY;
		uint32_t RAM_SIZE;
		uint32_t CACHE_CONTROL;
	};

	class MemoryControl : public BusDevice {
	public:
		MemoryControl();
		~MemoryControl();

		virtual void write(const std::string& busName, uint32_t address, uint32_t value, size_t valueSize) override;
		virtual uint32_t read(const std::string& busName, uint32_t address, size_t outputSize) override;
	private:
		IORegisters mIORegisters;
	};

}