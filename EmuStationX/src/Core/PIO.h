#pragma once

#include <array>

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {

	struct PIOIORegisters {
		
	};

	class PIO : public BusDevice {
	public:
		PIO();
		~PIO();

		virtual void write(const std::string& busName, uint32_t address, uint32_t value, size_t valueSize) override;
		virtual uint32_t read(const std::string& busName, uint32_t address, size_t outputSize) override;
	private:
		PIOIORegisters mIORegisters;
	};

}