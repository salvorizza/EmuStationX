#include "GPU.h"

namespace esx {

	GPU::GPU()
		: BusDevice(ESX_TEXT("GPU"))
	{
		addRange(ESX_TEXT("Root"), 0x1F801810, BYTE(0x9), 0xFFFFFFFF);
	}

	GPU::~GPU()
	{
	}

	void GPU::store(const StringView& busName, U32 address, U32 value)
	{
		ESX_CORE_LOG_WARNING("GPU - Writing to address {:08x} value {:08x}", address, value);
	}

	void GPU::load(const StringView& busName, U32 address, U32& output)
	{
		if (address == 0x1F801814) {
			//Enable DMA Block
			output = (1 << 28) | (1 << 27) | (1 << 26);
		}
		//ESX_CORE_LOG_WARNING("GPU - Reading address {:08x} not implemented yet", address);
	}

}