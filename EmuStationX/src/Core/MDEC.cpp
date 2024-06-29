#include "MDEC.h"

namespace esx {



	MDEC::MDEC()
		: BusDevice(ESX_TEXT("MDEC"))
	{
		addRange(ESX_TEXT("Root"), 0x1F801820, BYTE(8), 0xFFFFFFFF);
	}

	MDEC::~MDEC()
	{
	}

	void MDEC::clock(U64 clocks)
	{
	}

	void MDEC::store(const StringView& busName, U32 address, U32 value)
	{
		switch (address) {
			case 0x1F801820: {
				setCommandOrParameters(value);
				break;
			}
			case 0x1F801824: {
				setControlRegister(value);
				break;
			}
		}
	}

	void MDEC::load(const StringView& busName, U32 address, U32& output)
	{
		output = 0;

		switch (address) {
			case 0x1F801820: {
				output = getDataOrResponse();
				break;
			}
			case 0x1F801824: {
				output = getStatusRegister();
				break;
			}
		}
	}

	void MDEC::reset()
	{
		mStatusRegister = {};
		mControlRegister = {};
	}

	U32 MDEC::getStatusRegister()
	{
		U32 value = 0;

		value |= mStatusRegister.NumberOfParameterWords << 0;
		value |= mStatusRegister.CurrentBlock << 16;
		value |= mStatusRegister.DataOutputBit15Set << 23;
		value |= mStatusRegister.DataOutputSigned << 24;
		value |= (U8)mStatusRegister.DataOutputDepth << 25;
		value |= mStatusRegister.DataOutRequest << 27;
		value |= mStatusRegister.DataInRequest << 28;
		value |= mStatusRegister.CommandBusy << 29;
		value |= mStatusRegister.DataInFIFOFull << 30;
		value |= mStatusRegister.DataOutFIFOEmpty << 31;

		return value;
	}

	void MDEC::setStatusRegister(U32 value)
	{
		mStatusRegister.NumberOfParameterWords = (value >> 0) & 0xFFFF;
		mStatusRegister.CurrentBlock = (value >> 16) & 0x7;
		mStatusRegister.DataOutputBit15Set = (value >> 23) & 0x1;
		mStatusRegister.DataOutputSigned = (value >> 24) & 0x1;
		mStatusRegister.DataOutputDepth = (MDECOutputDepth)((value >> 25) & 0x3);
		mStatusRegister.DataOutRequest = (value >> 27) & 0x1;
		mStatusRegister.DataInRequest = (value >> 28) & 0x1;
		mStatusRegister.CommandBusy = (value >> 29) & 0x1;
		mStatusRegister.DataInFIFOFull = (value >> 30) & 0x1;
		mStatusRegister.DataOutFIFOEmpty = (value >> 31) & 0x1;
	}

	void MDEC::setControlRegister(U32 value)
	{
		mControlRegister.Reset = (value >> 31) & 0x1;
		mControlRegister.EnableDataInRequest = (value >> 30) & 0x1;
		mControlRegister.EnableDataOutRequest = (value >> 29) & 0x1;

		if (mControlRegister.Reset) {
			ESX_CORE_LOG_TRACE("TODO: MDEC Abort commands");
			setStatusRegister(0x80040000);
		}
	}

	U32 MDEC::getDataOrResponse()
	{
		return U32();
	}

	void MDEC::setCommandOrParameters(U32 value)
	{
	}

}