#include "RAM.h"

namespace esx {



	RAM::RAM(const StringView& name, U32 startAddress, U32 addressingSize, U64 size)
		: BusDevice(name)
	{
		mMemory.resize(size);
		reset();

		addRange(ESX_TEXT("Root"), startAddress, addressingSize, mMemory.size() - 1);
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
		*reinterpret_cast<U16*>(&mMemory[address]) = value;
	}

	void RAM::load(const StringView& busName, U32 address, U16& output)
	{
		output = *reinterpret_cast<U16*>(&mMemory[address]);
	}

	void RAM::store(const StringView& busName, U32 address, U32 value)
	{
		*reinterpret_cast<U32*>(&mMemory[address]) = value;
	}

	void RAM::load(const StringView& busName, U32 address, U32& output)
	{
		output = *reinterpret_cast<U32*>(&mMemory[address]);
	}

	void RAM::reset() {
		std::fill(mMemory.begin(), mMemory.end(), 0x00);
	}

}