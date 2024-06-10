#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {

	constexpr size_t I_STAT_ADDRESS = 0x1F801070;
	constexpr size_t I_MASK_ADDRESS = 0x1F801074;

	enum class InterruptType {
		VBlank = 1 << 0,
		GPU = 1 << 1,
		CDROM = 1 << 2,
		DMA = 1 << 3,
		Timer0 = 1 << 4,
		Timer1 = 1 << 5,
		Timer2 = 1 << 6,
		ControllerAndMemoryCard = 1 << 7,
		SIO = 1 << 8,
		SPU = 1 << 9,
		ControllerLightPen = 1 << 10
	};

	class InterruptControl : public BusDevice {
	public:
		InterruptControl();
		~InterruptControl();

		virtual void clock(U64 clocks) override;

		virtual void store(const StringView& busName, U32 address, U32 value) override;
		virtual void load(const StringView& busName, U32 address, U32& output) override;

		virtual void store(const StringView& busName, U32 address, U16 value) override;
		virtual void load(const StringView& busName, U32 address, U16& output) override;

		virtual void reset() override;

		void requestInterrupt(InterruptType type, BIT prevValue, BIT newValue, U64 delay = 0);

		BIT interruptPending();

	private:
		void setInterruptMask(U32 value);
		U32 getInterruptMask();

		void setInterruptStatus(U32 value);
		U32 getInterruptStatus();

	private:
		U16 mInterruptMask = 0;
		U16 mInterruptStatus = 0;
		Vector<Pair<U64, InterruptType>> mDelayedInterrupts;
	};

}