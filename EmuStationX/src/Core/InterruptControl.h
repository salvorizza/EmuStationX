#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {

	constexpr size_t I_STAT_ADDRESS = 0x1F801070;
	constexpr size_t I_MASK_ADDRESS = 0x1F801074;

	class InterruptControl : public BusDevice {
	public:
		InterruptControl();
		~InterruptControl();

		virtual void write(const std::string& busName, uint32_t address, uint32_t value, size_t valueSize) override;
		virtual uint32_t read(const std::string& busName, uint32_t address, size_t outputSize) override;
	};

}