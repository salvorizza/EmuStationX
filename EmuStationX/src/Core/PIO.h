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


		virtual void store(const StringView& busName, U32 address, U8 value) override;
		virtual void load(const StringView& busName, U32 address, U8& output) override;

	private:
		PIOIORegisters mIORegisters;
	};

}