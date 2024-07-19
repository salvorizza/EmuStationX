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
		if (mCurrentBlockPointer == -1 || mCurrentBlockPointer >= mCurrentBlock.size()) {
			if (mDecodedBlocks.size() == 0) return 0;

			mCurrentBlock = mDecodedBlocks.front();
			mDecodedBlocks.pop_front();
			mCurrentBlockPointer = 0;

			mStatusRegister.CurrentBlock = (mStatusRegister.CurrentBlock + 1) % 4;
		}

		return mCurrentBlock[mCurrentBlockPointer];
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
						decodeMacroblock();
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
				mDataIn.clear();
				mStatusRegister.DataInFIFOFull = ESX_FALSE;
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

	void MDEC::decodeMacroblock()
	{
		switch (mStatusRegister.DataOutputDepth) {
			case MDECOutputDepth::Bit15:
			case MDECOutputDepth::Bit24: {
				Array<Array<I16, 64>, 6> blocks = {};
				U64 src = 0;

				while (true) {
					BIT canDecode = ESX_TRUE;
					for (I32 i = 0; i < 6; i++) {
						if (!rl_decode_block(blocks[i], src, (i < 2) ? mQuantTableColor : mQuantTableLuminance)) canDecode = ESX_FALSE;
					}

					if (canDecode == ESX_FALSE) {
						break;
					}

					mDecodedBlocks.emplace_back();

					yuv_to_rgb(blocks[0], blocks[1], blocks[2], 0, 0);
					yuv_to_rgb(blocks[0], blocks[1], blocks[3], 0, 8);
					yuv_to_rgb(blocks[0], blocks[1], blocks[4], 8, 0);
					yuv_to_rgb(blocks[0], blocks[1], blocks[5], 8, 8);
				}

				break;
			}
		}
	}

	constexpr Array<U8, 64> zigzag = {
		0 ,1 ,5 ,6 ,14,15,27,28,
		2 ,4 ,7 ,13,16,26,29,42,
		3 ,8 ,12,17,25,30,41,43,
		9 ,11,18,24,31,40,44,53,
		10,19,23,32,39,45,52,54,
		20,22,33,38,46,51,55,60,
		21,34,37,47,50,56,59,61,
		35,36,48,49,57,58,62,63
	};

	static constexpr Array<U8, 64> calc_zagzig() {
		Array<U8, 64> result = {};
		for (I32 i = 0; i < 64; i++) {
			result[zigzag[i]] = i;
		}
		return result;
	}

	constexpr Array<U8, 64> zagzig = calc_zagzig();

	static constexpr I16 signed10bit(U16 n) {
		return static_cast<I16>(static_cast<U16>((n) << 5)) >> 5;
	}

	BIT MDEC::rl_decode_block(Array<I16, 64>& blk, U64& src, const Array<U8, 64>& qt)
	{
		std::fill(blk.begin(), blk.end(), 0);

		U16 n = 0;
		U64 k = 0;
		while (n != 0xFE00) {
			if (src < (mDataIn.size() * 2)) {
				n = reinterpret_cast<U16*>(mDataIn.data())[src++];
			} else {
				return ESX_FALSE;
			}
		}

		U16 q_scale = (n >> 10) & 0x3F;
		I32 val = signed10bit(n & 0x3FF) * qt[k];
		while (k < 64) {
			if (src >= (mDataIn.size() * 2)) {
				return ESX_FALSE;
			}

			if (q_scale == 0) val = signed10bit(n & 0x3FF) * 2;
			val = std::clamp(val, -0x400, 0x3FF);
			if (q_scale > 0) 
				blk[zagzig[k]] = val;
			else if (q_scale == 0) 
				blk[k] = val;
			n = reinterpret_cast<U16*>(mDataIn.data())[src++];
			k += ((n >> 10) & 0x3F) + 1;
			val = (signed10bit(n & 0x3FF) * qt[k] * q_scale + 4) / 8;
		}

		real_idct_core(blk);

		return ESX_TRUE;
	}

	void MDEC::real_idct_core(Array<I16, 64>& blk)
	{
		Array<I16, 64> temp = {};
		for (I32 pass = 0; pass < 1; pass++) {
			for (I32 x = 0; x < 7; x++) {
				for (I32 y = 0; y < 7; y++) {
					I32 sum = 0;
					for (I32 z = 0; z < 7; z++) {
						sum = sum + blk[y + z * 8] * (mScaleTable[x + z * 8] / 8);
					}
					temp[x + y * 8] = (sum + 0x0FFF) / 0x2000;
				}
			}
			std::swap(blk, temp);
		}
	}

	void MDEC::yuv_to_rgb(const Array<I16, 64>& Crblk, const Array<I16, 64>& Cbblk, const Array<I16, 64>& Yblk, U64 xx, U64 yy)
	{
		auto& rgb_block = mDecodedBlocks.front();
		for (I32 y = 0; y < 7; y++) {
			for (I32 x = 0; x < 7; x++) {
				U64 index = (((x + xx) / 2) + ((y + yy) / 2)) * 8;
				I32 R = Crblk[index];
				I32 B = Cbblk[index];
				I32 G = (-0.3437 * B) + (-0.7143 * R);
				R = (1.402 * R);
				B = (1.772 * B);
				I16 Y = Yblk[x + y * 8];
				R = std::clamp(Y + R, -128, 127);
				G = std::clamp(Y + G, -128, 127);
				B = std::clamp(Y + B, -128, 127);
				U32 BGR = ((static_cast<U32>(B) & 0xFF) << 16) | ((static_cast<U32>(G) & 0xFF) << 8) | ((static_cast<U32>(R) & 0xFF) << 0);
				if (mStatusRegister.DataOutputSigned == ESX_FALSE) BGR ^= 0x808080;
				rgb_block[(x + xx) + (y + yy) * 16] = BGR;
			}
		}
	}

}