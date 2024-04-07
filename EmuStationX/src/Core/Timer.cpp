#include "Timer.h"

#include "InterruptControl.h"
#include "R3000.h"

namespace esx {

	Timer::Timer()
		: BusDevice(ESX_TEXT("Timer"))
	{
		addRange(ESX_TEXT("Root"), 0x1F801100, BYTE(0x30), 0xFFFFFFFF);
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

	void Timer::systemClock()
	{
		mSystemClockDiv8 = (mSystemClockDiv8 + 1) % 8;

		if (mCounterModes[0].ClockSource == 0 || mCounterModes[0].ClockSource == 2) {
			incrementCounter(0);
		}

		if (mCounterModes[1].ClockSource == 0 || mCounterModes[1].ClockSource == 2) {
			incrementCounter(1);
		}

		if (mCounterModes[2].ClockSource == 0 || mCounterModes[2].ClockSource == 1) {
			incrementCounter(2);
		} else {
			if (mSystemClockDiv8 == 0) {
				incrementCounter(2);
			}
		}
	}

	void Timer::hblank()
	{
		if (mCounterModes[1].ClockSource == 1 || mCounterModes[1].ClockSource == 3) {
			incrementCounter(1);
		}
	}

	void Timer::vblank()
	{
	}

	void Timer::dot()
	{
		if (mCounterModes[0].ClockSource == 1 || mCounterModes[0].ClockSource == 3) {
			incrementCounter(0);
		}
	}

	void Timer::setCurrentValue(U8 counter, U32 value)
	{
		mCurrentValues[counter] = value & 0xFFFF;
	}

	U32 Timer::getCurrentValue(U8 counter)
	{
		return mCurrentValues[counter];
	}

	void Timer::setCounterMode(U8 counter, U32 value)
	{
		mCounterModes[counter].SyncEnable = (value >> 0) & 0x1;
		mCounterModes[counter].SyncMode = (value >> 1) & 0x3;
		mCounterModes[counter].ResetCounter = (value >> 3) & 0x1;
		mCounterModes[counter].IRQCounterEqualTargetEnable = (value >> 4) & 0x1;
		mCounterModes[counter].IRQCounterEqualMaxEnable = (value >> 5) & 0x1;
		mCounterModes[counter].IRQRepeat = (value >> 6) & 0x1;
		mCounterModes[counter].IRQToggle = (value >> 7) & 0x1;
		mCounterModes[counter].ClockSource = (value >> 8) & 0x3;

		mCounterModes[counter].InterruptRequest = ESX_FALSE;
		mCurrentValues[counter] = 0x0000;
	}

	U32 Timer::getCounterMode(U8 counter)
	{
		U32 result = 0;

		result |= (mCounterModes[counter].SyncEnable << 0);
		result |= (mCounterModes[counter].SyncMode << 1);
		result |= (mCounterModes[counter].ResetCounter << 3);
		result |= (mCounterModes[counter].IRQCounterEqualTargetEnable << 4);
		result |= (mCounterModes[counter].IRQCounterEqualMaxEnable << 5);
		result |= (mCounterModes[counter].IRQRepeat << 6);
		result |= (mCounterModes[counter].IRQToggle << 7);
		result |= (mCounterModes[counter].ClockSource << 8);
		result |= ((mCounterModes[counter].InterruptRequest == ESX_TRUE ? 0 : 1) << 10);
		result |= (mCounterModes[counter].ReachedTargetValue << 11);
		result |= (mCounterModes[counter].ReachedMax << 12);

		mCounterModes[counter].ReachedTargetValue = ESX_FALSE;
		mCounterModes[counter].ReachedMax = ESX_FALSE;

		return result;
	}

	void Timer::setTargetValue(U8 counter, U32 value)
	{
		mTargetValues[counter] = value & 0xFFFF;
	}

	U32 Timer::getTargetValue(U8 counter)
	{
		return mTargetValues[counter];
	}

	void Timer::incrementCounter(U8 counter)
	{
		mCurrentValues[counter]++;
		BIT reachedTargetValue = mCurrentValues[counter] == mTargetValues[counter];
		BIT reachedMaxValue = mCurrentValues[counter] == 0xFFFF;

		mCounterModes[counter].ReachedTargetValue = reachedTargetValue;
		mCounterModes[counter].ReachedMax = reachedMaxValue;

		if ((mCounterModes[counter].ResetCounter == 1 && reachedTargetValue) || (mCounterModes[counter].ResetCounter == 0 && reachedMaxValue)) {
			mCurrentValues[counter] = 0x0000;
		}

		if ((mCounterModes[counter].IRQCounterEqualTargetEnable && reachedTargetValue) || (mCounterModes[counter].IRQCounterEqualMaxEnable && reachedMaxValue)) {
			//TODO: this is Repeat mode do pulse and toggle
			BIT newInterruptRequest = !mCounterModes[counter].InterruptRequest;

			SharedPtr<InterruptControl> ic = getBus("Root")->getDevice<InterruptControl>("InterruptControl");
			ic->requestInterrupt((InterruptType)((U8)InterruptType::Timer0 << counter), !mCounterModes[counter].InterruptRequest, !newInterruptRequest);

			mCounterModes[counter].InterruptRequest = newInterruptRequest;
		}
	}

}