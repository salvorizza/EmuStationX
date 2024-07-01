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
		mCurrentCommand = MDECCommand::None;
		mDataIn.clear();
		mDataOut.clear();
	}

	void MDEC::channelIn(U32 word)
	{
		setCommandOrParameters(word);
	}

	U32 MDEC::channelOut()
	{
		return 0;
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
			mDataIn.clear();
			mDataOut.clear();
		}
	}

	U32 MDEC::getDataOrResponse()
	{
		ESX_CORE_LOG_ERROR("TODO: MDEC Data");

		return U32();
	}

	void MDEC::setCommandOrParameters(U32 value)
	{
		if (mStatusRegister.NumberOfParameterWords == 0xFFFF || mCurrentCommand == MDECCommand::NoFunction || mCurrentCommand == MDECCommand::None) {
			U8 command = (value >> 29) & 0x7;

			mStatusRegister.DataOutputBit15Set = (value >> 25) & 0x1;
			mStatusRegister.DataOutputSigned = (value >> 26) & 0x1;
			mStatusRegister.DataOutputDepth = (MDECOutputDepth)((value >> 27) & 0x3);

			mCurrentCommand = (command >= 1 && command <= 3) ? (MDECCommand)command : MDECCommand::NoFunction;
			switch (mCurrentCommand) {
				case MDECCommand::DecodeMacroblock: {
					mStatusRegister.NumberOfParameterWords = (value & 0xFFFF) - 1;
					break;
				}

				case MDECCommand::SetQuantTable: {
					BIT Color = (value >> 0) & 0x1;
					mStatusRegister.NumberOfParameterWords = (16 + (Color ? 16 : 0)) - 1;
					break;
				}

				case MDECCommand::SetScaleTable: {
					mStatusRegister.NumberOfParameterWords = 32 - 1;
					break;
				}

				case MDECCommand::NoFunction: {
					mStatusRegister.NumberOfParameterWords = value & 0xFFFF;
					mStatusRegister.DataInFIFOFull = ESX_TRUE;
					break;
				}
			}

			mStatusRegister.DataInFIFOFull = ESX_FALSE;
			mStatusRegister.CommandBusy = ESX_TRUE;

			ESX_CORE_LOG_TRACE("MDEC - Command {} {}", command, mStatusRegister.NumberOfParameterWords);
		} else {
			//ESX_CORE_LOG_TRACE("MDEC - Parameter {:08x}h", value);
			mDataIn.push_back(value);

			mStatusRegister.NumberOfParameterWords--;
			if (mStatusRegister.NumberOfParameterWords == 0xFFFF) {
				mStatusRegister.DataInFIFOFull = ESX_TRUE;
				
				switch (mCurrentCommand) {
					case MDECCommand::DecodeMacroblock: {
						ESX_CORE_LOG_ERROR("MDEC::DecodeMacroblock not implemented yet");
						break;
					}

					case MDECCommand::SetQuantTable: {
						setQuantTable();
						break;
					}

					case MDECCommand::SetScaleTable: {
						setScaleTable();
						break;
					}
				}

				mStatusRegister.CommandBusy = ESX_FALSE;
			}
		}

	}

	void MDEC::setQuantTable()
	{
		for (I32 i = 0; i < 32; i++) {
			mQuantTableLuminance[i] = reinterpret_cast<U8*>(mDataIn.data())[i];
		}

		if (mDataIn.size() > 16) {
			for (I32 i = 0; i < 32; i++) {
				mQuantTableColor[i] = reinterpret_cast<U8*>(mDataIn.data())[32 + i];
			}
		}
	}

	void MDEC::setScaleTable()
	{
		for (I32 i = 0; i < 64; i++) {
			mScaleTable[i] = reinterpret_cast<I16*>(mDataIn.data())[i];
		}
	}

}