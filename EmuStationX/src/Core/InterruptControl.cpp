#include "InterruptControl.h"

#include "R3000.h"

namespace esx {



	InterruptControl::InterruptControl()
		: BusDevice(ESX_TEXT("InterruptControl"))
	{
		addRange(ESX_TEXT("Root"), I_STAT_ADDRESS, BYTE(4), 0xFFFFFFFF);
		addRange(ESX_TEXT("Root"), I_MASK_ADDRESS, BYTE(4), 0xFFFFFFFF);

	}

	InterruptControl::~InterruptControl()
	{
	}

	void InterruptControl::clock(U64 clocks)
	{
		for (const auto& [targetClocks, interruptType] : mDelayedInterrupts) {
			if (clocks >= targetClocks) {
				mInterruptStatus |= (U32)interruptType;
			}
		}
		std::erase_if(mDelayedInterrupts, [&](const Pair<U64, InterruptType>& pair) { return clocks >= pair.first; });
	}

	void InterruptControl::store(const StringView& busName, U32 address, U32 value)
	{
		switch (address) {
			case I_STAT_ADDRESS: {
				setInterruptStatus(value);
				break;
			}
			case I_MASK_ADDRESS: {
				setInterruptMask(value);
				break;
			}
		}
	}

	void InterruptControl::load(const StringView& busName, U32 address, U32& output)
	{
		output = 0;

		switch (address) {
			case I_STAT_ADDRESS: {
				output = getInterruptStatus();
				break;
			}
			case I_MASK_ADDRESS: {
				output = getInterruptMask();
				break;
			}
		}
	}

	void InterruptControl::store(const StringView& busName, U32 address, U16 value)
	{

		switch (address) {
			case I_STAT_ADDRESS: {
				setInterruptStatus(value);
				break;
			}
			case I_MASK_ADDRESS: {
				setInterruptMask(value);
				break;
			}
		}
	}

	void InterruptControl::load(const StringView& busName, U32 address, U16& output)
	{

		switch (address) {
			case I_STAT_ADDRESS: {
				output = getInterruptStatus();
				break;
			}
			case I_MASK_ADDRESS: {
				output = getInterruptMask();
				break;
			}
		}
	}

	void InterruptControl::requestInterrupt(InterruptType type, BIT prevValue, BIT newValue, U64 delay)
	{

		if (prevValue == ESX_FALSE && newValue == ESX_TRUE) {
			U64 clocks = getBus("Root")->getDevice<R3000>("R3000")->getClocks();
			mDelayedInterrupts.emplace_back(clocks + delay, type);
		}
	}

	BIT InterruptControl::interruptPending()
	{
		return (mInterruptStatus & mInterruptMask);
	}

	void InterruptControl::setInterruptMask(U32 value)
	{
		value &= 0x7FF;
		mInterruptMask = value;
	}

	U32 InterruptControl::getInterruptMask()
	{
		return mInterruptMask;
	}

	void InterruptControl::setInterruptStatus(U32 value)
	{
		value &= 0x7FF;
		mInterruptStatus &= value;
	}

	U32 InterruptControl::getInterruptStatus()
	{
		return mInterruptStatus;
	}

}