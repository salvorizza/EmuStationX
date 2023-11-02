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


		virtual void store(const String& busName, U32 address, U8 value) override;
		virtual void load(const String& busName, U32 address, U8& output) override;
	private:
		PIOIORegisters mIORegisters;
	};

}