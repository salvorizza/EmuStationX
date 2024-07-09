#include "Timer.h"

#include "InterruptControl.h"
#include "R3000.h"

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
		Counter& timer0 = mCounters[0];
		Counter& timer1 = mCounters[1];
		Counter& timer2 = mCounters[2];

		if (!timer0.Pause && (timer0.Mode.ClockSource & 1) == 0) {
			incrementCounter(timer0);
		}

		if (!timer1.Pause && (timer1.Mode.ClockSource & 1) == 0) {
			incrementCounter(timer1);
		}

		if (!timer2.Pause) {
			if (timer2.Mode.ClockSource < 2) {
				incrementCounter(timer2);
			}
			else {
				if ((clocks % 8) == 0) {
					incrementCounter(timer2);
				}
			}
		}
	}

	void Timer::hblank()
	{
		Counter& timer0 = mCounters[0];
		Counter& timer1 = mCounters[1];

		if (timer0.Mode.SyncEnable) {
			if (timer0.Mode.SyncMode == 1) {
				timer0.CurrentValue = 0x0000;
			} else if (timer0.Mode.SyncMode == 3) {
				timer0.Mode.SyncEnable = ESX_FALSE;
			}
		}

		if (!timer1.Pause && (timer1.Mode.ClockSource & 1) == 1) {
			incrementCounter(timer1);
		}
	}

	void Timer::vblank()
	{
		Counter& timer1 = mCounters[1];

		if (timer1.Mode.SyncEnable) {
			if (timer1.Mode.SyncMode == 1) {
				timer1.CurrentValue = 0x0000;
			}
			else if (timer1.Mode.SyncMode == 3) {
				timer1.Mode.SyncEnable = ESX_FALSE;
			}
		}
	}

	void Timer::dot()
	{
		Counter& timer0 = mCounters[0];

		if (!timer0.Pause && (timer0.Mode.ClockSource & 1) == 1) {
			incrementCounter(timer0);
		}
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
		mCounters[counter].Mode.SyncEnable = (value >> 0) & 0x1;
		mCounters[counter].Mode.SyncMode = (value >> 1) & 0x3;
		mCounters[counter].Mode.ResetCounter = (value >> 3) & 0x1;
		mCounters[counter].Mode.IRQCounterEqualTargetEnable = (value >> 4) & 0x1;
		mCounters[counter].Mode.IRQCounterEqualMaxEnable = (value >> 5) & 0x1;
		mCounters[counter].Mode.IRQRepeat = (value >> 6) & 0x1;
		mCounters[counter].Mode.IRQToggle = (value >> 7) & 0x1;
		mCounters[counter].Mode.ClockSource = (value >> 8) & 0x3;

		mCounters[counter].Mode.InterruptRequest = ESX_FALSE;
		mCounters[counter].CurrentValue = 0x0000;

		if (counter == 0 && mCounters[counter].Mode.SyncEnable && mCounters[counter].Mode.SyncMode == 3) mCounters[counter].Pause = ESX_TRUE;
		if (counter == 1 && mCounters[counter].Mode.SyncEnable && mCounters[counter].Mode.SyncMode == 3) mCounters[counter].Pause = ESX_TRUE;
		if (counter == 2 && mCounters[counter].Mode.SyncEnable && (mCounters[counter].Mode.SyncMode & 1) == 0) mCounters[counter].Pause = ESX_TRUE;
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

	void Timer::incrementCounter(Counter& timer)
	{
		CounterModeRegister& modeRegister = timer.Mode;

		timer.CurrentValue++;
		BIT reachedTargetValue = timer.CurrentValue == timer.TargetValue;
		BIT reachedMaxValue = timer.CurrentValue == 0xFFFF;

		modeRegister.ReachedTargetValue = reachedTargetValue;
		modeRegister.ReachedMax = reachedMaxValue;

		if ((modeRegister.ResetCounter && reachedTargetValue) || (!modeRegister.ResetCounter && reachedMaxValue)) {
			timer.CurrentValue = 0x0000;
		}

		if ((modeRegister.IRQCounterEqualTargetEnable && reachedTargetValue) || (modeRegister.IRQCounterEqualMaxEnable && reachedMaxValue)) {
			handleInterrupt(timer, modeRegister);
		}
	}

	void Timer::handleInterrupt(Counter& timer, CounterModeRegister& modeRegister)
	{
		static const InterruptType interruptTypes[] = {
			InterruptType::Timer0,
			InterruptType::Timer1,
			InterruptType::Timer2
		};

		if (!modeRegister.IRQRepeat) {
			ESX_CORE_LOG_WARNING("Timer {} one-shot mode not implemented yet", timer.Number);
		}

		if (!modeRegister.IRQToggle) {
			ESX_CORE_LOG_WARNING("Timer {} pulse mode not implemented yet", timer.Number);
		}

		//TODO: this is Repeat mode do pulse and toggle
		BIT newInterruptRequest = !modeRegister.InterruptRequest;

		SharedPtr<InterruptControl> ic = getBus("Root")->getDevice<InterruptControl>("InterruptControl");
		ic->requestInterrupt(interruptTypes[timer.Number], !modeRegister.InterruptRequest, !newInterruptRequest);

		modeRegister.InterruptRequest = newInterruptRequest;
	}

}