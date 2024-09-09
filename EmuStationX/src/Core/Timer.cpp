#include "Timer.h"

#include "InterruptControl.h"
#include "R3000.h"
#include "GPU.h"

#include "optick.h"

namespace esx {

	Timer::Timer()
		: BusDevice(ESX_TEXT("Timer"))
	{
		addRange(ESX_TEXT("Root"), 0x1F801100, BYTE(0x30), 0xFFFFFFFF);
		reset();
	}

	Timer::~Timer()
	{
	}

	void Timer::store(const StringView& busName, U32 address, U16 value)
	{
		U8 counter = (address >> 4) & 0xF;
		address &= 0xFFFFFF0F;

		switch (address) {
			case 0x1F801100: {
				setCurrentValue(counter, value);
				break;
			}
			case 0x1F801104: {
				setCounterMode(counter, value);
				break;
			}
			case 0x1F801108: {
				setTargetValue(counter, value);
				break;
			}
		}
	}

	void Timer::load(const StringView& busName, U32 address, U16& output)
	{
		U8 counter = (address >> 4) & 0xF;
		address &= 0xFFFFFF0F;

		switch (address) {
			case 0x1F801100: {
				output = getCurrentValue(counter);
				break;
			}

			case 0x1F801104: {
				output = getCounterMode(counter);
				break;
			}

			case 0x1F801108: {
				output = getTargetValue(counter);
				break;
			}

			default: {
				ESX_CORE_LOG_ERROR("Timer Address load16 {:08X}h not handled yet", address);
				break;
			}
		}

	}

	void Timer::store(const StringView& busName, U32 address, U32 value)
	{
		U8 counter = (address >> 4) & 0xF;
		address &= 0xFFFFFF0F;

		switch (address) {
			case 0x1F801100: {
				setCurrentValue(counter, value);
				break;
			}
			case 0x1F801104: {
				setCounterMode(counter, value);
				break;
			}
			case 0x1F801108: {
				setTargetValue(counter, value);
				break;
			}
		}
	}


	void Timer::load(const StringView& busName, U32 address, U32& output)
	{
		U8 counter = (address >> 4) & 0xF;
		address &= 0xFFFFFF0F;

		switch (address) {
			case 0x1F801100: {
				output = getCurrentValue(counter);
				break;
			}
			case 0x1F801104: {
				output = getCounterMode(counter);
				break;
			}
			case 0x1F801108: {
				output = getTargetValue(counter);
				break;
			}
		}
	}

	void Timer::init()
	{
		mCPU = getBus("Root")->getDevice<R3000>("R3000");
		mGPU = getBus("Root")->getDevice<GPU>("GPU");
	}

	void Timer::reset()
	{
		mCounters = {};
		U32 i = 0;
		for (Counter& counter : mCounters) {
			counter.Number = i;
			i++;
		}
	}

	void Timer::clock(U64 clocks)
	{
		for (Counter& timer : mCounters) {
			if (timer.Mode.IRQCounterEqualTargetEnable && clocks >= timer.ScheduledClockToTarget) {
				incrementCounter(timer, CalculateDistance(timer.TargetValue, timer.CurrentValue));

				timer.ScheduledClockToTargetStart = mCPU->getClocks();
				timer.ScheduledClockToTarget = PreCalculateTimerScheduleClock(GetClockSource(timer.Number, timer.Mode.ClockSource), timer.CurrentValue, timer.TargetValue);
			}
		}

		for (Counter& timer : mCounters) {
			if (timer.Mode.IRQCounterEqualMaxEnable && clocks >= timer.ScheduledClockToMax) {
				incrementCounter(timer, CalculateDistance(0xFFFF, timer.CurrentValue));

				timer.ScheduledClockToMaxStart = mCPU->getClocks();
				timer.ScheduledClockToMax = PreCalculateTimerScheduleClock(GetClockSource(timer.Number, timer.Mode.ClockSource), timer.CurrentValue, 0xFFFF);
			}
		}
	}

	void Timer::startHblank()
	{
		startSync();
	}

	void Timer::endHblank()
	{
		endSync();
	}

	void Timer::startVblank()
	{
		startSync();
	}

	void Timer::endVblank()
	{
		endSync();
	}

	void Timer::dot()
	{
	}

	void Timer::setCurrentValue(U8 counter, U32 value)
	{
		mCounters[counter].CurrentValue = value & 0xFFFF;
	}

	U32 Timer::getCurrentValue(U8 counter)
	{
		return mCounters[counter].CurrentValue;
	}

	void Timer::setCounterMode(U8 counter, U32 value)
	{
		Counter& timer = mCounters[counter];

		CounterSyncMode syncMode = GetSyncMode(counter, timer.Mode.SyncMode);
		ClockSource clockSource = GetClockSource(counter, timer.Mode.ClockSource);
		U64 divider = GetDivider(clockSource);


		U64 distance = (timer.ScheduledClockToMax - timer.ScheduledClockToMaxStart);
		if (clockSource == ClockSource::Hblank || clockSource == ClockSource::Dotclock) {
			distance = GPU::ToGPUClock(distance);
		}
		U16 increment = static_cast<U16>(distance / divider);
		incrementCounter(timer, increment);

		timer.Mode.SyncEnable = (value >> 0) & 0x1;
		timer.Mode.SyncMode = (value >> 1) & 0x3;
		timer.Mode.ResetCounter = (value >> 3) & 0x1;
		timer.Mode.IRQCounterEqualTargetEnable = (value >> 4) & 0x1;
		timer.Mode.IRQCounterEqualMaxEnable = (value >> 5) & 0x1;
		timer.Mode.IRQRepeat = (value >> 6) & 0x1;
		timer.Mode.IRQToggle = (value >> 7) & 0x1;
		timer.Mode.ClockSource = (value >> 8) & 0x3;

		timer.Mode.InterruptRequest = !mCounters[counter].Mode.IRQToggle;

		timer.CurrentValue = 0x0000;


		if (timer.Mode.SyncEnable && (syncMode == CounterSyncMode::ResetAndPauseOutside || syncMode == CounterSyncMode::PauseUntilThenFreeRun || syncMode == CounterSyncMode::Stop)) timer.Pause = ESX_TRUE;

		timer.IRQHappened = ESX_FALSE;

		timer.ScheduledClockToTargetStart = mCPU->getClocks();
		timer.ScheduledClockToTarget = PreCalculateTimerScheduleClock(clockSource, timer.CurrentValue, timer.TargetValue);

		timer.ScheduledClockToMaxStart = mCPU->getClocks();
		timer.ScheduledClockToMax = PreCalculateTimerScheduleClock(clockSource, timer.CurrentValue, 0xFFFF);
	}

	U32 Timer::getCounterMode(U8 counter)
	{
		U32 result = 0;

		result |= (mCounters[counter].Mode.SyncEnable << 0);
		result |= (mCounters[counter].Mode.SyncMode << 1);
		result |= (mCounters[counter].Mode.ResetCounter << 3);
		result |= (mCounters[counter].Mode.IRQCounterEqualTargetEnable << 4);
		result |= (mCounters[counter].Mode.IRQCounterEqualMaxEnable << 5);
		result |= (mCounters[counter].Mode.IRQRepeat << 6);
		result |= (mCounters[counter].Mode.IRQToggle << 7);
		result |= (mCounters[counter].Mode.ClockSource << 8);
		result |= ((mCounters[counter].Mode.InterruptRequest == ESX_TRUE ? 0 : 1) << 10);
		result |= (mCounters[counter].Mode.ReachedTargetValue << 11);
		result |= (mCounters[counter].Mode.ReachedMax << 12);

		mCounters[counter].Mode.ReachedTargetValue = ESX_FALSE;
		mCounters[counter].Mode.ReachedMax = ESX_FALSE;

		return result;
	}

	void Timer::setTargetValue(U8 counter, U32 value)
	{
		mCounters[counter].TargetValue = value & 0xFFFF;
	}

	U32 Timer::getTargetValue(U8 counter)
	{
		return mCounters[counter].TargetValue;
	}

	void Timer::incrementCounters(ClockSource clockSource)
	{
		for (Counter& counter : mCounters) {
			if (!counter.Pause && GetClockSource(counter.Number, counter.Mode.ClockSource) == clockSource) {
				incrementCounter(counter);
			}
		}
	}

	void Timer::startSync()
	{
		for (Counter& counter : mCounters) {
			if (counter.Mode.SyncEnable) {
				startCounterSync(counter);
			}
		}
	}

	void Timer::endSync()
	{
		for (Counter& counter : mCounters) {
			if (counter.Mode.SyncEnable) {
				endCounterSync(counter);
			}
		}
	}

	void Timer::incrementCounter(Counter& timer, U16 increment)
	{
		CounterModeRegister& modeRegister = timer.Mode;

		timer.CurrentValue += increment;
		BIT reachedTargetValue = timer.CurrentValue == timer.TargetValue;
		BIT reachedMaxValue = timer.CurrentValue == 0xFFFF;

		if ((modeRegister.ResetCounter && reachedTargetValue) || (!modeRegister.ResetCounter && reachedMaxValue)) {
			timer.CurrentValue = 0x0000;
		}

		modeRegister.ReachedTargetValue = reachedTargetValue;
		modeRegister.ReachedMax = reachedMaxValue;

		if ((modeRegister.IRQCounterEqualTargetEnable && reachedTargetValue) || (modeRegister.IRQCounterEqualMaxEnable && reachedMaxValue)) {
			handleInterrupt(timer, modeRegister);
		}
	}

	void Timer::startCounterSync(Counter& counter)
	{
		CounterSyncMode syncMode = GetSyncMode(counter.Number, counter.Mode.SyncMode);

		switch (syncMode) {
			case CounterSyncMode::PauseDuring: {
				counter.Pause = ESX_TRUE;
				break;
			}
			case CounterSyncMode::Reset: {
				counter.CurrentValue = 0x0000;
				break;
			}
			case CounterSyncMode::ResetAndPauseOutside: {
				counter.CurrentValue = 0x0000;
				counter.Pause = ESX_FALSE;
				break;
			}
			case CounterSyncMode::PauseUntilThenFreeRun: {
				counter.Mode.SyncEnable = ESX_FALSE;
				counter.Pause = ESX_FALSE;
				break;
			}
		}
	}

	void Timer::endCounterSync(Counter& counter)
	{
		CounterSyncMode syncMode = GetSyncMode(counter.Number, counter.Mode.SyncMode);

		switch (syncMode) {
			case CounterSyncMode::PauseDuring: {
				counter.Pause = ESX_FALSE;
				break;
			}
			case CounterSyncMode::ResetAndPauseOutside: {
				counter.Pause = ESX_TRUE;
				break;
			}
		}
	}

	void Timer::handleInterrupt(Counter& timer, CounterModeRegister& modeRegister)
	{
		static const InterruptType interruptTypes[] = {
			InterruptType::Timer0,
			InterruptType::Timer1,
			InterruptType::Timer2
		};

		if (!modeRegister.IRQRepeat && timer.IRQHappened) {
			return;
		}

		BIT newInterruptRequest = !modeRegister.InterruptRequest;

		SharedPtr<InterruptControl> ic = getBus("Root")->getDevice<InterruptControl>("InterruptControl");
		ic->requestInterrupt(interruptTypes[timer.Number], !modeRegister.InterruptRequest, !newInterruptRequest);

		if (modeRegister.IRQToggle) {
			modeRegister.InterruptRequest = newInterruptRequest;
		}

		timer.IRQHappened = ESX_TRUE;
	}

	U64 Timer::PreCalculateTimerScheduleClock(ClockSource clockSource, U16 CurrentValue, U16 TargetValue)
	{
		U64 clocks = mCPU->getClocks();

		U16 distance = CalculateDistance(TargetValue, CurrentValue);
		U64 divider = GetDivider(clockSource);

		U64 clocksToAdd = distance * divider;
		if (clockSource == ClockSource::Dotclock || clockSource == ClockSource::Hblank) {
			clocksToAdd = GPU::FromGPUClock(clocksToAdd);
		}

		return clocks + clocksToAdd;
	}

	U64 Timer::GetDivider(ClockSource clockSource)
	{
		switch (clockSource) {
			case ClockSource::SystemClock: {
				return 1;
			}

			case ClockSource::SystemClockDiv8: {
				return 8;
			}

			case ClockSource::Hblank: {
				return mGPU->GetClocksPerScanline();
			}

			case ClockSource::Dotclock: {
				return mGPU->GetDotClocks();
			}
		}
	}

}