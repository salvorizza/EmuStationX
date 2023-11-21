#include "Timer.h"

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
		ESX_CORE_LOG_WARNING("Timer - Writing to address {:08x} not implemented yet", address);
	}

	void Timer::load(const StringView& busName, U32 address, U32& value)
	{
		ESX_CORE_LOG_WARNING("Timer - Reading address {:08x} not implemented yet", address);
	}

	void Timer::store(const StringView& busName, U32 address, U32 value)
	{
		ESX_CORE_LOG_WARNING("Timer - Writing to address {:08x} not implemented yet", address);
	}

}