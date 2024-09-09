#pragma once

#include <array>

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {

	enum class ClockSource {
		SystemClock,
		SystemClockDiv8,
		Dotclock,
		Hblank
	};

	enum class CounterSyncMode {
		FreeRun,
		PauseDuring,
		Reset,
		ResetAndPauseOutside,
		PauseUntilThenFreeRun,
		Stop
	};

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

	struct Counter {
		U32 Number = 0;
		CounterModeRegister Mode = {};
		U16 CurrentValue = 0;
		U16 TargetValue = 0;
		BIT Pause = ESX_FALSE;
		BIT IRQHappened = ESX_FALSE;

		U64 ScheduledClockToMaxStart = 0;
		U64 ScheduledClockToTargetStart = 0;

		U64 ScheduledClockToMax = 0;
		U64 ScheduledClockToTarget = 0;
	};

	typedef class GPU;
	typedef class R3000;

	static constexpr Array<Array<ClockSource, 4>, 3> COUNTER_CLOCK_SOURCES = { {
		{ ClockSource::SystemClock, ClockSource::Dotclock, ClockSource::SystemClock, ClockSource::Dotclock },
		{ ClockSource::SystemClock, ClockSource::Hblank, ClockSource::SystemClock, ClockSource::Hblank },
		{ ClockSource::SystemClock, ClockSource::SystemClock, ClockSource::SystemClockDiv8, ClockSource::SystemClockDiv8 }
	}};

	static constexpr Array<Array<CounterSyncMode, 4>, 3> COUNTER_SYNC_MODES = { {
		{ CounterSyncMode::PauseDuring, CounterSyncMode::Reset, CounterSyncMode::ResetAndPauseOutside, CounterSyncMode::PauseUntilThenFreeRun },
		{ CounterSyncMode::PauseDuring, CounterSyncMode::Reset, CounterSyncMode::ResetAndPauseOutside, CounterSyncMode::PauseUntilThenFreeRun },
		{ CounterSyncMode::Stop, CounterSyncMode::FreeRun, CounterSyncMode::FreeRun, CounterSyncMode::Stop }
	} };

	class Timer : public BusDevice {
	public:
		Timer();
		~Timer();

		virtual void store(const StringView& busName, U32 address, U16 value);
		virtual void load(const StringView& busName, U32 address, U16& output);

		virtual void store(const StringView& busName, U32 address, U32 value);
		virtual void load(const StringView& busName, U32 address, U32& output);

		virtual void init() override;
		virtual void reset() override;

		virtual void clock(U64 clocks) override;

		void startHblank();
		void endHblank();

		void startVblank();
		void endVblank();

		void dot();

	private:
		void setCurrentValue(U8 counter, U32 value);
		U32 getCurrentValue(U8 counter);

		void setCounterMode(U8 counter, U32 value);
		U32 getCounterMode(U8 counter);

		void setTargetValue(U8 counter, U32 value);
		U32 getTargetValue(U8 counter);

		void incrementCounters(ClockSource clockSource);
		void startSync();
		void endSync();

		void incrementCounter(Counter& counter, U16 increment = 1);
		void startCounterSync(Counter& counter);
		void endCounterSync(Counter& counter);

		void handleInterrupt(Counter& timer, CounterModeRegister& modeRegister);

		U64 PreCalculateTimerScheduleClock(ClockSource clockSource, U16 CurrentValue, U16 TargetValue);

		U64 GetDivider(ClockSource clockSource);
		static constexpr ClockSource GetClockSource(U8 counter, U8 clockSource) { return COUNTER_CLOCK_SOURCES[counter][clockSource]; }
		static constexpr CounterSyncMode GetSyncMode(U8 counter, U8 syncMode) { return COUNTER_SYNC_MODES[counter][syncMode]; }
		static constexpr U16 CalculateDistance(U16 TargetValue, U16 CurrentValue) { return (CurrentValue < TargetValue) ? (TargetValue - CurrentValue) : (0xFFFF - (CurrentValue - TargetValue) + 1); }

	private:
		Array<Counter, 3> mCounters;
		SharedPtr<GPU> mGPU;
		SharedPtr<R3000> mCPU;
	};

}