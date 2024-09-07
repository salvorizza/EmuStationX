#include "Bios.h"

namespace esx {



	Bios::Bios(const StringView& path)
		:	BusDevice(ESX_TEXT("Bios")),
			mPath(path)
	{
		addRange(ESX_TEXT("Root"), 0x1FC00000, KIBI(512), 0x7FFFF);
		mMemory.reserve(KIBI(512));

		reset();

	}

	Bios::~Bios()
	{
	}

	void Bios::load(const StringView& busName, U32 address, U8& output)
	{
		output = mMemory[address];
	}

	void Bios::load(const StringView& busName, U32 address, U32& output)
	{
		output = *reinterpret_cast<U32*>(&mMemory[address]);
	}

	void Bios::load(const StringView& busName, U32 address, U16& output)
	{
		output = *reinterpret_cast<U16*>(&mMemory[address]);
	}

	void Bios::reset()
	{
		std::ifstream input(mPath.data(), std::ios::binary);
		mMemory.resize(0);
		mMemory.insert(mMemory.begin(), std::istreambuf_iterator<char>(input), {});
		input.close();
	}

	U8* Bios::getFastPointer(U32 address)
	{
		return &mMemory[address];
	}
}