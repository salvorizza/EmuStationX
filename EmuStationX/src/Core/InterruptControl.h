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

		virtual void store(const StringView& busName, U32 address, U32 value) override;
		virtual void load(const StringView& busName, U32 address, U32& output) override;

		virtual void store(const StringView& busName, U32 address, U16 value) override;
		virtual void load(const StringView& busName, U32 address, U16& output) override;
	};

}