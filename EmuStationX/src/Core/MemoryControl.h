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
	};

	class MemoryControl : public BusDevice {
	public:
		MemoryControl();
		~MemoryControl();


		virtual void store(const StringView& busName, U32 address, U32 value) override;
	private:
		IORegisters mIORegisters;
	};

}