#pragma once

#include <array>

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {


	struct CounterModeRegister {
		BIT SyncEnable = ESX_FALSE;
		U8 SyncMode = 0;
		BIT ResetCounter = ESX_FALSE;
		BIT IRQCounterEqualTargetEnable = ESX_FALSE;
		BIT IRQCounterEqualMaxEnable = ESX_FALSE;
		BIT IRQRepeat = ESX_FALSE;
		BIT IRQToggle = ESX_FALSE;
		U8 ClockSource = 0;
		BIT InterruptRequest = ESX_FALSE;
		BIT ReachedTargetValue = ESX_FALSE;
		BIT ReachedMax = ESX_FALSE;
	};

	class Timer : public BusDevice {
	public:
		Timer();
		~Timer();

		virtual void store(const StringView& busName, U32 address, U16 value);
		virtual void load(const StringView& busName, U32 address, U16& output);

		virtual void store(const StringView& busName, U32 address, U32 value);
		virtual void load(const StringView& busName, U32 address, U32& output);

		void systemClock();
		void hblank();
		void vblank();
		void dot();

	private:
		void setCurrentValue(U8 counter, U32 value);
		U32 getCurrentValue(U8 counter);

		void setCounterMode(U8 counter, U32 value);
		U32 getCounterMode(U8 counter);

		void setTargetValue(U8 counter, U32 value);
		U32 getTargetValue(U8 counter);

		void incrementCounter(U8 counter);


	private:
		Array<U16, 3> mCurrentValues;
		Array<CounterModeRegister, 3> mCounterModes;
		Array<U16, 3> mTargetValues;

		U64 mSystemClockDiv8;
	};

}