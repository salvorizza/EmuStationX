#include "RAM.h"

namespace esx {



	RAM::RAM()
		: BusDevice(ESX_TEXT("RAM"))
	{
		mMemory.resize(KIBI(2048));

		addRange(ESX_TEXT("Root"), 0x00000000, KIBI(2048), 0x1FFFFF);
	}

	RAM::~RAM()
	{
	}

	void RAM::store(const StringView& busName, U32 address, U8 value)
	{
		mMemory[address] = value;
	}

	void RAM::load(const StringView& busName, U32 address, U8& output)
	{
		output = mMemory[address];
	}

	void RAM::store(const StringView& busName, U32 address, U16 value)
	{
		for (size_t i = 0; i < sizeof(U16); i++) {
			mMemory[address + i] = (value & 0xFF);
			value >>= 8;
		}
	}

	void RAM::load(const StringView& busName, U32 address, U16& output)
	{
		output = 0;
		for (size_t i = 0; i < sizeof(U16); i++) {
			output <<= 8;
			output |= mMemory[address + (sizeof(U16) - 1 - i)];
		}
	}

	void RAM::store(const StringView& busName, U32 address, U32 value)
	{
		for (size_t i = 0; i < sizeof(U32); i++) {
			mMemory[address + i] = (value & 0xFF);
			value >>= 8;
		}
	}

	void RAM::load(const StringView& busName, U32 address, U32& output)
	{
		output = 0;
		for (size_t i = 0; i < sizeof(U32); i++) {
			output <<= 8;
			output |= mMemory[address + (sizeof(U32) - 1 - i)];
		}
	}

}