#include "Timer.h"

#include "InterruptControl.h"
#include "R3000.h"
#include "GPU.h"

#include "optick.h"

#include "Core/Scheduler.h"

namespace esx {

	Timer::Timer()
		: BusDevice(ESX_TEXT("Timer"))
	{
		addRange(ESX_TEXT("Root"), 0x1F801100, BYTE(0x30), 0xFFFFFFFF);

		auto reachTarget = [&](const SchedulerEvent& ev) {
			U8 counter = ev.Read<U8>();
			Counter& timer = mCounters[counter];
			incrementCounter(timer, CalculateDistance(timer.TargetValue, timer.CurrentValue));
			RescheduleTargetEvent(timer, GetClockSource(timer.Number, timer.Mode.ClockSource), ESX_FALSE);
			RescheduleMaxEvent(timer, GetClockSource(timer.Number, timer.Mode.ClockSource));
		};



		auto reachMax = [&](const SchedulerEvent& ev) {
			U8 counter = ev.Read<U8>();
			Counter& timer = mCounters[counter];
			incrementCounter(timer, CalculateDistance(0xFFFF, timer.CurrentValue));
			RescheduleMaxEvent(timer, GetClockSource(timer.Number, timer.Mode.ClockSource), ESX_FALSE);
			RescheduleTargetEvent(timer, GetClockSource(timer.Number, timer.Mode.ClockSource));
		};

		Scheduler::AddSchedulerEventHandler(SchedulerEventType::Timer0ReachTarget, reachTarget);
		Scheduler::AddSchedulerEventHandler(SchedulerEventType::Timer1ReachTarget, reachTarget);
		Scheduler::AddSchedulerEventHandler(SchedulerEventType::Timer2ReachTarget, reachTarget);

		Scheduler::AddSchedulerEventHandler(SchedulerEventType::Timer0ReachMax, reachMax);
		Scheduler::AddSchedulerEventHandler(SchedulerEventType::Timer1ReachMax, reachMax);
		Scheduler::AddSchedulerEventHandler(SchedulerEventType::Timer2ReachMax, reachMax);
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
			RescheduleMaxEvent(counter, GetClockSource(counter.Number, counter.Mode.ClockSource));
		}
	}

	void Timer::clock(U64 clocks)
	{
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

		RescheduleTargetEvent(mCounters[counter], GetClockSource(counter, mCounters[counter].Mode.ClockSource));
		RescheduleMaxEvent(mCounters[counter], GetClockSource(counter, mCounters[counter].Mode.ClockSource));
	}

	U32 Timer::getCurrentValue(U8 counter)
	{
		calculateCurrentValue(mCounters[counter]);

		return mCounters[counter].CurrentValue;
	}

	void Timer::setCounterMode(U8 counter, U32 value)
	{
		Counter& timer = mCounters[counter];

		CounterSyncMode syncMode = GetSyncMode(counter, timer.Mode.SyncMode);
		ClockSource clockSource = GetClockSource(counter, timer.Mode.ClockSource);
		
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

		RescheduleTargetEvent(timer, clockSource);
		RescheduleMaxEvent(timer, clockSource);
	}

	U32 Timer::getCounterMode(U8 counter)
	{
		U32 result = 0;

		calculateCurrentValue(mCounters[counter]);

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

		RescheduleTargetEvent(mCounters[counter], GetClockSource(counter, mCounters[counter].Mode.ClockSource));
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
		ESX_CORE_LOG_ERROR(__FUNCTION__);
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
		ESX_CORE_LOG_ERROR(__FUNCTION__);

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

	void Timer::calculateCurrentValue(Counter& timer)
	{
		SchedulerEventType timerEventType = static_cast<SchedulerEventType>(static_cast<U8>(SchedulerEventType::Timer0ReachMax) + timer.Number);
		auto event = Scheduler::NextEventOfType(timerEventType);

		U64 ScheduledClockToMaxStart = 0;
		if (event) {
			ScheduledClockToMaxStart = event.value().ClockStart;
		}

		U64 currentClocks = mCPU->getClocks();
		CounterSyncMode syncMode = GetSyncMode(timer.Number, timer.Mode.SyncMode);
		ClockSource clockSource = GetClockSource(timer.Number, timer.Mode.ClockSource);
		U64 divider = GetDivider(clockSource);

		U64 distance = (currentClocks - ScheduledClockToMaxStart);
		if (clockSource == ClockSource::Hblank || clockSource == ClockSource::Dotclock) {
			distance = GPU::ToGPUClock(distance);
		}
		U16 increment = static_cast<U16>(distance / divider);
		incrementCounter(timer, increment);
		
		if (event) {
			SchedulerEvent timerEventMax = {
				.Type = timerEventType,
				.ClockStart = currentClocks,
				.ClockTarget = event.value().ClockTarget
			};
			timerEventMax.Write<U8>(static_cast<U8>(timer.Number));

			Scheduler::UnScheduleAllEvents(timerEventType);
			Scheduler::ScheduleEvent(timerEventMax);
		}
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

	void Timer::RescheduleTargetEvent(Counter& timer, ClockSource clockSource, BIT unschedule)
	{
		if (timer.Mode.IRQCounterEqualTargetEnable) {
			SchedulerEventType type = static_cast<SchedulerEventType>(static_cast<U8>(SchedulerEventType::Timer0ReachTarget) + timer.Number);
			U64 clockStart = mCPU->getClocks();
			U64 clockTarget = PreCalculateTimerScheduleClock(clockSource, timer.CurrentValue, timer.TargetValue);

			SchedulerEvent timerEventTarget = {
				.Type = type,
				.ClockStart = clockStart,
				.ClockTarget = clockTarget,
			};
			timerEventTarget.Write<U8>(static_cast<U8>(timer.Number));

			if (unschedule) Scheduler::UnScheduleAllEvents(type);
			Scheduler::ScheduleEvent(timerEventTarget);
		}
	}

	void Timer::RescheduleMaxEvent(Counter& timer, ClockSource clockSource, BIT unschedule)
	{
		SchedulerEventType type = static_cast<SchedulerEventType>(static_cast<U8>(SchedulerEventType::Timer0ReachMax) + timer.Number);
		U64 clockStart = mCPU->getClocks();
		U64 clockTarget = PreCalculateTimerScheduleClock(clockSource, timer.CurrentValue, 0xFFFF);
		if (clockStart != clockTarget) {
			SchedulerEvent timerEventMax = {
				.Type = type,
				.ClockStart = clockStart,
				.ClockTarget = clockTarget,
			};
			timerEventMax.Write<U8>(static_cast<U8>(timer.Number));

			if (unschedule) Scheduler::UnScheduleAllEvents(type);
			Scheduler::ScheduleEvent(timerEventMax);
		}
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