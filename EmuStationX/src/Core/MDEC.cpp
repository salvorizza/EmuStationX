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
		U32 word = 0;

		if (mDataOut.size() != 0) {
			word = mDataOut.front();
			mDataOut.pop_front();
		}

		if (mDataOut.size() == 0 && mDecoding) {
			switch (mStatusRegister.DataOutputDepth) {
			case MDECOutputDepth::Bit15:
			case MDECOutputDepth::Bit24: {
				if (!decode_colored_macroblock()) {
					mDecoding = ESX_FALSE;
				}
				break;
			}
			}
			if (mDecoding) {
				copy_to_out();
			} else {
				mDataIn.clear();
			}
		}

		mStatusRegister.DataOutFIFOEmpty = mDataOut.size() == 0;

		return word;
	}

	U32 MDEC::getStatusRegister()
	{
		U32 value = 0;

		mStatusRegister.DataInRequest = mControlRegister.EnableDataInRequest && mDataIn.size() == 0;
		mStatusRegister.DataOutRequest = mControlRegister.EnableDataOutRequest && mDataOut.size() != 0;

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
		U32 word = 0;
		if (mDataOut.size() != 0) {
			word = mDataOut.front();
			mDataOut.pop_front();
		}
		mStatusRegister.DataOutFIFOEmpty = mDataOut.size() == 0;
		return word;
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
				mStatusRegister.DataInFIFOFull = ESX_FALSE;
			}
		}

	}

	void MDEC::setQuantTable()
	{
		for (I32 i = 0; i < 64; i++) {
			mQuantTableLuminance[i] = reinterpret_cast<U8*>(mDataIn.data())[i];
		}

		if (mDataIn.size() > 16) {
			for (I32 i = 0; i < 64; i++) {
				mQuantTableColor[i] = reinterpret_cast<U8*>(mDataIn.data())[64 + i];
			}
		}

		mDataIn.clear();
	}

	void MDEC::setScaleTable()
	{
		for (I32 i = 0; i < 64; i++) {
			mScaleTable[i] = reinterpret_cast<I16*>(mDataIn.data())[i];
		}

		mDataIn.clear();
	}

	void MDEC::decodeMacroblock()
	{
		mCurrentSrc = 0;
		mDecoding = ESX_TRUE;

		switch (mStatusRegister.DataOutputDepth) {
			case MDECOutputDepth::Bit15:
			case MDECOutputDepth::Bit24: {
				if (!decode_colored_macroblock()) {
					mDecoding = ESX_FALSE;
				}
				break;
			}
		}
		if (mDecoding) {
			copy_to_out();
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

	constexpr Array<F32, 8> scalefactor = {
		1.000000000f, 1.387039845f, 1.306562965f, 1.175875602f,
		1.000000000f, 0.785694958f, 0.541196100f, 0.275899379f
	};

	constexpr Array<U8, 64> calc_zagzig() {
		Array<U8, 64> result = {};
		for (I32 i = 0; i < 64; i++) {
			result[zigzag[i]] = i;
		}
		return result;
	}

	constexpr Array<F32, 64> calc_scalezag() {
		Array<F32, 64> result = {};
		for (I32 x = 0; x < 8; x++) {
			for (I32 y = 0; y < 8; y++) {
				result[zigzag[x + y * 8]] = scalefactor[x] * scalefactor[y] / 8;
			}
		}
		return result;
	}


	constexpr Array<U8, 64> zagzig = calc_zagzig();
	constexpr Array<F32, 64> scalezag = calc_scalezag();

	constexpr I16 signed10bit(U16 n) {
		return static_cast<I16>(n << 6) >> 6;
	}

	constexpr I32 signed9bit(I32 n) {
		return static_cast<I32>(n << 23) >> 23;
	}

	BIT MDEC::decode_colored_macroblock()
	{
		Array<Array<I16, 64>, 6> blocks = {};

		BIT canDecode = ESX_TRUE;
		for (I32 i = 0; i < 6; i++) {
			if (!rl_decode_block(blocks[i], mCurrentSrc, (i < 2) ? mQuantTableColor : mQuantTableLuminance)) canDecode = ESX_FALSE;
		}

		if (canDecode == ESX_FALSE) {
			return ESX_FALSE;
		}

		yuv_to_rgb(blocks[0], blocks[1], blocks[2], 0, 0);
		yuv_to_rgb(blocks[0], blocks[1], blocks[3], 0, 8);
		yuv_to_rgb(blocks[0], blocks[1], blocks[4], 8, 0);
		yuv_to_rgb(blocks[0], blocks[1], blocks[5], 8, 8);

		return ESX_TRUE;
	}

	BIT MDEC::rl_decode_block(Array<I16, 64>& blk, U64& src, const Array<U8, 64>& qt)
	{
		std::fill(blk.begin(), blk.end(), 0);

		U16 n = 0;
		U64 k = 0;
		do {
			if (src >= (mDataIn.size() * 2)) {
				return ESX_FALSE;
			}
			n = reinterpret_cast<U16*>(mDataIn.data())[src++];
		} while (n == 0xFE00);

		U16 q_scale = (n >> 10) & 0x3F;
		I32 val = signed10bit(n & 0x3FF) * qt[k];
		if (q_scale == 0) val = signed10bit(n & 0x3FF) * 2;
		val = std::clamp(val, -0x400, 0x3FF);
		val = val * scalezag[k]; //fast_idct_core only
		blk[(q_scale > 0) ? zagzig[k] : k] = val;

		while (k < 64) {
			if (src >= (mDataIn.size() * 2)) {
				return ESX_FALSE;
			}
			n = reinterpret_cast<U16*>(mDataIn.data())[src++];
			k += ((n >> 10) & 0x3F) + 1;
			if (k < 64) {
				val = (signed10bit(n & 0x3FF) * qt[k] * q_scale + 4) / 8;
				if (q_scale == 0) val = signed10bit(n & 0x3FF) * 2;
				val = std::clamp(val, -0x400, 0x3FF);
				blk[(q_scale > 0) ? zagzig[k] : k] = val;
			}
			
			if (k >= 63) {
				k = 64;
			}
		}

		fast_idct_core(blk);

		//real_idct_core(blk);

		return ESX_TRUE;
	}

	void MDEC::fast_idct_core(Array<I16, 64>& blk)
	{
		Array<I16, 64> src = blk;
		Array<I16, 64> dst = {};

		for (I32 pass = 0; pass < 2; pass++) {
			for (I32 i = 0; i < 8; i++) {
				BIT isColumnZero = ESX_TRUE;
				for (I32 k = 0; k < 8; k++) {
					if (src[k * 8 + i] != 0) {
						isColumnZero = ESX_FALSE;
						break;
					}
				}

				if (isColumnZero) {
					for (I32 k = 0; k < 8; k++) {
						dst[i * 8 + k] = src[0 * 8 + i];
					}
				}
				else {
					F32 z10 = src[0 * 8 + i] + src[4 * 8 + i]; F32 z11 = src[0 * 8 + i] - src[4 * 8 + i];
					F32 z13 = src[2 * 8 + i] + src[6 * 8 + i]; F32 z12 = src[2 * 8 + i] - src[6 * 8 + i];
					z12 = (1.414213562f * z12) - z13;
					F32 tmp0 = z10 + z13; F32 tmp3 = z10 - z13; F32 tmp1 = z11 + z12; F32 tmp2 = z11 - z12;
					z13 = src[3 * 8 + i] + src[5 * 8 + i]; z10 = src[3 * 8 + i] - src[5 * 8 + i];
					z11 = src[1 * 8 + i] + src[7 * 8 + i]; z12 = src[1 * 8 + i] - src[7 * 8 + i];
					F32 z5 = (1.847759065f * (z12 - z10));

					F32 tmp7 = z11 + z13;
					F32 tmp6 = (2.613125930f * (z10)) + z5 - tmp7;
					F32 tmp5 = (1.414213562f * (z11 - z13)) - tmp6;
					F32 tmp4 = (1.082392200f * (z12)) - z5 + tmp5;
					dst[i * 8 + 0] = tmp0 + tmp7; dst[i * 8 + 7] = tmp0 - tmp7;
					dst[i * 8 + 1] = tmp1 + tmp6; dst[i * 8 + 6] = tmp1 - tmp6;
					dst[i * 8 + 2] = tmp2 + tmp5; dst[i * 8 + 5] = tmp2 - tmp5;
					dst[i * 8 + 4] = tmp3 + tmp4; dst[i * 8 + 3] = tmp3 - tmp4;
				}
			}
			std::swap(src, dst);
		}
		blk = src;
	}

	void MDEC::real_idct_core(Array<I16, 64>& blk)
	{
		Array<I16, 64> src = blk;
		Array<I16, 64> dst = {};

		for (I32 pass = 0; pass < 2; pass++) {
			for (I32 x = 0; x < 8; x++) {
				for (I32 y = 0; y < 8; y++) {
					I32 sum = 0;
					for (I32 z = 0; z < 8; z++) {
						sum += src[y + z * 8] * (mScaleTable[x + z * 8] / 8);
					}
					dst[x + y * 8] = (sum + 0x0FFF) / 0x2000;
				}
			}
			std::swap(src, dst);
		}

		blk = src;
	}

	void MDEC::yuv_to_rgb(const Array<I16, 64>& Crblk, const Array<I16, 64>& Cbblk, const Array<I16, 64>& Yblk, U64 xx, U64 yy)
	{
		for (I32 y = 0; y < 8; y++) {
			for (I32 x = 0; x < 8; x++) {
				U64 index = ((x + xx) / 2) + ((y + yy) / 2) * 8;
				I16 R = Crblk[index];
				I16 B = Cbblk[index];
				I16 G = static_cast<I16>((-0.3437f * static_cast<F32>(B)) + (-0.7143f * static_cast<F32>(R)));

				R = static_cast<I16>((1.402f * static_cast<F32>(R)));
				B = static_cast<I16>((1.772f * static_cast<F32>(B)));

				I32 Y = Yblk[x + y * 8];
				R = static_cast<I16>(std::clamp(Y + R, -128, 127));
				G = static_cast<I16>(std::clamp(Y + G, -128, 127));
				B = static_cast<I16>(std::clamp(Y + B, -128, 127));

				U32 BGR = (static_cast<U8>(B & 0xFF) << 16) | (static_cast<U8>(G & 0xFF) << 8) | (static_cast<U8>(R & 0xFF) << 0);
				if (mStatusRegister.DataOutputSigned == ESX_FALSE) BGR ^= 0x808080;
				mCurrentDecodedBlock[(x + xx) + (y + yy) * 16] = BGR;
			}
		}
	}

	void MDEC::copy_to_out()
	{
		if (mStatusRegister.DataOutputDepth == MDECOutputDepth::Bit24) {
			for (I32 i = 0; i < mCurrentDecodedBlock.size(); i++) {
				U32 word = mCurrentDecodedBlock[i];

				if (mTidyPack24BitBytesToFill != 0) {
					U32 bytesToAppend = (word & (0xFFFFFF >> ((3 - mTidyPack24BitBytesToFill) * 8))) << ((4 - mTidyPack24BitBytesToFill) * 8);
					mDataOut.back() = mDataOut.back() | bytesToAppend;
					word >>= (mTidyPack24BitBytesToFill * 8);
				}

				if (mTidyPack24BitBytesToFill != 3) {
					mDataOut.emplace_back(word);
				}

				mTidyPack24BitBytesToFill = (mTidyPack24BitBytesToFill + 1) % 4;
			}
		}
		else {
			U16 a = mStatusRegister.DataOutputBit15Set;
			for (I32 i = 0; i < mCurrentDecodedBlock.size();) {
				U32 word1 = mCurrentDecodedBlock[i++];
				U8 r = (word1 >> 3) & 0x1F;
				U8 g = (word1 >> 11) & 0x1F;
				U8 b = (word1 >> 19) & 0x1F;
				U16 pixel1 = (a << 15) | (b << 10) | (g << 5) | r;


				U32 word2 = mCurrentDecodedBlock[i++];
				r = (word2 >> 3) & 0x1F;
				g = (word2 >> 11) & 0x1F;
				b = (word2 >> 19) & 0x1F;
				U16 pixel2 = (a << 15) | (b << 10) | (g << 5) | r;

				mDataOut.emplace_back((pixel2 << 16) | pixel1);
			}
		}

		mStatusRegister.DataOutFIFOEmpty = mDataOut.size() == 0;
	}

}