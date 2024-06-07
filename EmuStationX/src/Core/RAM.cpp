#include "RAM.h"

namespace esx {



	RAM::RAM(const StringView& name, U32 startAddress, U32 endAddress, U64 size)
		: BusDevice(name)
	{
		mMemory.resize(size);

		addRange(ESX_TEXT("Root"), startAddress, endAddress, mMemory.size() - 1);
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