#include "Bios.h"

namespace esx {



	Bios::Bios(const String& path)
		: BusDevice(ESX_TEXT("Bios"))
	{
		std::ifstream input(path, std::ios::binary);
		mMemory = Vector<U8>(std::istreambuf_iterator<char>(input), {});
		input.close();

		addRange(ESX_TEXT("Root"), 0x1FC00000, KIBI(512), 0x7FFFF);
	}

	Bios::~Bios()
	{
	}

	void Bios::load(const String& busName, U32 address, U8& output)
	{
		output = mMemory[address];
	}

	void Bios::load(const String& busName, U32 address, U32& output)
	{
		output = 0;

		for (size_t i = 0; i < sizeof(U32); i++) {
			output <<= 8;
			output |= mMemory[address + (sizeof(U32) - 1 - i)];
		}
	}

}