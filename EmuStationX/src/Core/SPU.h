#pragma once

#include <array>

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {

	struct SPUIORegisters {
	};

	class SPU : public BusDevice {
	public:
		SPU();
		~SPU();

		virtual void store(const String& busName, U32 address, U16 value) override;
		virtual void load(const String& busName, U32 address, U16& output) override;
	private:
		SPUIORegisters mIORegisters;
	};

}