#include "GTE.h"

namespace esx {
	void GTE::clock() {
		if (mCurrentCommand.RemainingClocks < mCurrentCommand.TotalClocks) {
			mCurrentCommand.RemainingClocks++;
		}
	}

	void GTE::command(U32 command) {
		mRegisters.FLAG = 0;
		mCurrentCommand = decodeCommand(command);
		(this->*mCurrentCommand.Execute)();
		if ((mRegisters.FLAG & 0x7F87E000) != 0) {
			mRegisters.FLAG |= FlagRegisterError;
		}
	}

	U32 GTE::getRegister(U32 index)
	{
		switch (index) {
			case 15: {
				return mRegisters.Registers[14];
			}

			case 28:
			case 29: {
				const U8 r = static_cast<U8>(std::clamp(mRegisters.IR1 / 0x80, 0x00, 0x1F));
				const U8 g = static_cast<U8>(std::clamp(mRegisters.IR2 / 0x80, 0x00, 0x1F));
				const U8 b = static_cast<U8>(std::clamp(mRegisters.IR3 / 0x80, 0x00, 0x1F));
				return r | (g << 5) | (b << 10);
			}

			case 58: {
				return static_cast<U32>(static_cast<I32>(static_cast<I16>(mRegisters.H))); //Bug
			}

			default: {
				return mRegisters.Registers[index];
			}
		}
	}

	void GTE::setRegister(U32 index, U32 value)
	{
		switch (index) {
			case 1: 
			case 3: 
			case 5: 
			case 8: 
			case 9: 
			case 10:
			case 11:
			case 36:
			case 44:
			case 52:
			case 58:
			case 59:
			case 61:
			case 62: {
				mRegisters.Registers[index] = static_cast<I32>(static_cast<I16>(static_cast<U16>(value)));
				break;
			}

			case 7:
			case 16:
			case 17:
			case 18:
			case 19: {
				mRegisters.Registers[index] = static_cast<U32>(static_cast<U16>(value));
				break;
			}

			case 15: {
				mRegisters.Registers[12] = mRegisters.Registers[13];
				mRegisters.Registers[13] = mRegisters.Registers[14];
				mRegisters.Registers[14] = value;
				break;
			}

			case 28: {
				mRegisters.IRGB = value & 0x7FFF;
				mRegisters.Registers[9] = static_cast<I32>(static_cast<I16>(((value >> 0) & 0x1F) * 0x80));
				mRegisters.Registers[10] = static_cast<I32>(static_cast<I16>(((value >> 5) & 0x1F) * 0x80));
				mRegisters.Registers[11] = static_cast<I32>(static_cast<I16>(((value >> 10) & 0x1F) * 0x80));
				break;
			}

			case 30: {
				mRegisters.LZCS = static_cast<I32>(value);
				mRegisters.LZCR = (mRegisters.LZCS < 0) ? std::countl_one(value) : std::countl_zero(value);
				break;
			}

			case 29:
			case 31: {
				break;
			}

			case 63: {
				mRegisters.FLAG = value & 0x7FFFF000;
				if ((mRegisters.FLAG & 0x7F87E000) != 0) {
					mRegisters.FLAG |= FlagRegisterError;
				}
				break;
			}

			default: {
				mRegisters.Registers[index] = value;
				break;
			}
		}
	}

	static const Array<U8, 0x101> unr_table = {
		0xFF,0xFD,0xFB,0xF9,0xF7,0xF5,0xF3,0xF1,0xEF,0xEE,0xEC,0xEA,0xE8,0xE6,0xE4,0xE3,
		0xE1,0xDF,0xDD,0xDC,0xDA,0xD8,0xD6,0xD5,0xD3,0xD1,0xD0,0xCE,0xCD,0xCB,0xC9,0xC8,
		0xC6,0xC5,0xC3,0xC1,0xC0,0xBE,0xBD,0xBB,0xBA,0xB8,0xB7,0xB5,0xB4,0xB2,0xB1,0xB0,
		0xAE,0xAD,0xAB,0xAA,0xA9,0xA7,0xA6,0xA4,0xA3,0xA2,0xA0,0x9F,0x9E,0x9C,0x9B,0x9A,
		0x99,0x97,0x96,0x95,0x94,0x92,0x91,0x90,0x8F,0x8D,0x8C,0x8B,0x8A,0x89,0x87,0x86,
		0x85,0x84,0x83,0x82,0x81,0x7F,0x7E,0x7D,0x7C,0x7B,0x7A,0x79,0x78,0x77,0x75,0x74,
		0x73,0x72,0x71,0x70,0x6F,0x6E,0x6D,0x6C,0x6B,0x6A,0x69,0x68,0x67,0x66,0x65,0x64,
		0x63,0x62,0x61,0x60,0x5F,0x5E,0x5D,0x5D,0x5C,0x5B,0x5A,0x59,0x58,0x57,0x56,0x55,
		0x54,0x53,0x53,0x52,0x51,0x50,0x4F,0x4E,0x4D,0x4D,0x4C,0x4B,0x4A,0x49,0x48,0x48,
		0x47,0x46,0x45,0x44,0x43,0x43,0x42,0x41,0x40,0x3F,0x3F,0x3E,0x3D,0x3C,0x3C,0x3B,
		0x3A,0x39,0x39,0x38,0x37,0x36,0x36,0x35,0x34,0x33,0x33,0x32,0x31,0x31,0x30,0x2F,
		0x2E,0x2E,0x2D,0x2C,0x2C,0x2B,0x2A,0x2A,0x29,0x28,0x28,0x27,0x26,0x26,0x25,0x24,
		0x24,0x23,0x22,0x22,0x21,0x20,0x20,0x1F,0x1E,0x1E,0x1D,0x1D,0x1C,0x1B,0x1B,0x1A,
		0x19,0x19,0x18,0x18,0x17,0x16,0x16,0x15,0x15,0x14,0x14,0x13,0x12,0x12,0x11,0x11,
		0x10,0x0F,0x0F,0x0E,0x0E,0x0D,0x0D,0x0C,0x0C,0x0B,0x0A,0x0A,0x09,0x09,0x08,0x08,
		0x07,0x07,0x06,0x06,0x05,0x05,0x04,0x04,0x03,0x03,0x02,0x02,0x01,0x01,0x00,0x00,
		0x00
	};

	void GTE::reset()
	{
		mRegisters = {};
		mCurrentCommand = {};
	}

	void GTE::RTPS() {
		I32 H_CALC = 0;
		Internal_RTPS(mRegisters.V0, mCurrentCommand.Saturate, mCurrentCommand.ShiftFraction, H_CALC);

		I64 MAC0 = H_CALC * I64(mRegisters.DQA) + I64(mRegisters.DQB);
		overflowCheckMAC(0, MAC0, ESX_FALSE);

		I64 MAC012 = MAC0 >> 12;
		if (MAC012 < 0x0000 || MAC012 > 0x1000) mRegisters.FLAG |= FlagRegisterIR0Sat;
		mRegisters.IR0 = std::clamp<I32>(MAC012, 0x0000, 0x1000);
	}

	void GTE::NCLIP() {
		I64 MAC0 =	I64(mRegisters.SXY0[0]) * I64(mRegisters.SXY1[1]) + 
					I64(mRegisters.SXY1[0]) * I64(mRegisters.SXY2[1]) + 
					I64(mRegisters.SXY2[0]) * I64(mRegisters.SXY0[1]) - 
					I64(mRegisters.SXY0[0]) * I64(mRegisters.SXY2[1]) - 
					I64(mRegisters.SXY1[0]) * I64(mRegisters.SXY0[1]) - 
					I64(mRegisters.SXY2[0]) * I64(mRegisters.SXY1[1]);
		overflowCheckMAC(0, MAC0, ESX_FALSE);
	}

	void GTE::OP() {
		I64 D1 = I64(mRegisters.RT[0 * 3 + 0]);
		I64 D2 = I64(mRegisters.RT[1 * 3 + 1]);
		I64 D3 = I64(mRegisters.RT[2 * 3 + 2]);

		I64 MAC1 = mRegisters.IR3 * D2 - mRegisters.IR2 * D3;
		I64 MAC2 = mRegisters.IR1 * D3 - mRegisters.IR3 * D1;
		I64 MAC3 = mRegisters.IR2 * D1 - mRegisters.IR1 * D2;

		overflowCheckMAC(1, MAC1, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(2, MAC2, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(3, MAC3, mCurrentCommand.ShiftFraction);

		overfowCheckIR(1, mRegisters.MACV[0], mCurrentCommand.Saturate);
		overfowCheckIR(2, mRegisters.MACV[1], mCurrentCommand.Saturate);
		overfowCheckIR(3, mRegisters.MACV[2], mCurrentCommand.Saturate);
	}

	void GTE::DPCS() {
		Internal_DPCS(mRegisters.RGBC);
	}

	void GTE::INTPL() {
		U64 MAC1_orig = ((U64)mRegisters.IR1 << 12);
		U64 MAC2_orig = ((U64)mRegisters.IR2 << 12);
		U64 MAC3_orig = ((U64)mRegisters.IR3 << 12);

		/*[MAC1,MAC2,MAC((U64)mRegisters.IR2 << 12)3] = [IR1,IR2,IR3] SHL 12 */
		/*[IR1,IR2,IR3] ((U64)mRegisters.IR3 << 12)= (([RFC,GFC,BFC] SHL 12) - [MAC1,MAC2,MAC3]) SAR (sf*12)*/
		I64 MAC1 = ((I64)mRegisters.FC[0] << 12) - MAC1_orig;
		I64 MAC2 = ((I64)mRegisters.FC[1] << 12) - MAC2_orig;
		I64 MAC3 = ((I64)mRegisters.FC[2] << 12) - MAC3_orig;

		overflowCheckMAC(1, MAC1, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(2, MAC2, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(3, MAC3, mCurrentCommand.ShiftFraction);

		overfowCheckIR(1, mRegisters.MACV[0], ESX_FALSE);
		overfowCheckIR(2, mRegisters.MACV[1], ESX_FALSE);
		overfowCheckIR(3, mRegisters.MACV[2], ESX_FALSE);

		/*[MAC1,MAC2,MAC3] = (([IR1,IR2,IR3] * IR0) + [MAC1,MAC2,MAC3])*/
		MAC1 = (mRegisters.IR1 * mRegisters.IR0) + MAC1_orig;
		MAC2 = (mRegisters.IR2 * mRegisters.IR0) + MAC2_orig;
		MAC3 = (mRegisters.IR3 * mRegisters.IR0) + MAC3_orig;

		overflowCheckMAC(1, MAC1, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(2, MAC2, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(3, MAC3, mCurrentCommand.ShiftFraction);

		/*Color FIFO = [MAC1/16,MAC2/16,MAC3/16,CODE]*/
		pushColor(mRegisters.MACV[0] >> 4, mRegisters.MACV[1] >> 4, mRegisters.MACV[2] >> 4);

		/*[IR1,IR2,IR3] = [MAC1,MAC2,MAC3]*/
		overfowCheckIR(1, mRegisters.MACV[0], mCurrentCommand.Saturate);
		overfowCheckIR(2, mRegisters.MACV[1], mCurrentCommand.Saturate);
		overfowCheckIR(3, mRegisters.MACV[2], mCurrentCommand.Saturate);
	}

	void GTE::MVMVA() {
		Array<I32, 3> T = {};
		Array<I16, 3> V = {};
		Array<I16, 9> M = {};

		Array<I16, 3> IR = { (I16)mRegisters.IR1, (I16)mRegisters.IR2, (I16)mRegisters.IR3 };
		Array<I32, 3> Zero = { 0,0,0 };
		Array<I16, 9> GarbageMatrix = {
			I16(I32(-1) * I16(U16(mRegisters.RGBC[0]) << 4)), I16(U16(mRegisters.RGBC[0]) << 4), I16(mRegisters.IR0),
			mRegisters.RT[0 * 3 + 2],mRegisters.RT[0 * 3 + 2],mRegisters.RT[0 * 3 + 2],
			mRegisters.RT[1 * 3 + 1],mRegisters.RT[1 * 3 + 1],mRegisters.RT[1 * 3 + 1]
		};

		I64 MAC1, MAC2, MAC3;

		switch (mCurrentCommand.MultiplyMatrix) {
			case MultiplyMatrix::Rotation: {
				M = mRegisters.RT;
				break;
			}
			case MultiplyMatrix::Light: {
				M = mRegisters.LLM;
				break;
			}
			case MultiplyMatrix::Color: {
				M = mRegisters.LCM;
				break;
			}
			case MultiplyMatrix::Reserved: {
				M = GarbageMatrix;
				break;
			}
		}

		switch (mCurrentCommand.MultiplyVector) {
			case MultiplyVector::V0: {
				V = mRegisters.V0;
				break;
			}
			case MultiplyVector::V1: {
				V = mRegisters.V1;
				break;
			}
			case MultiplyVector::V2: {
				V = mRegisters.V2;
				break;
			}
			case MultiplyVector::IR_Long: {
				V = IR;
				break;
			}
		}

		switch (mCurrentCommand.TranslationVector) {
			case TranslationVector::TR: {
				T = mRegisters.TR;
				break;
			}
			case TranslationVector::BK: {
				T = mRegisters.BK;
				break;
			}
			case TranslationVector::FC_Bugged: {
				T = mRegisters.FC;
				break;
			}
		}

		if (mCurrentCommand.TranslationVector == TranslationVector::FC_Bugged) {
			MAC1 = I64(M[0 * 3 + 1]) * V[1] + I64(M[0 * 3 + 2]) * V[2];
			MAC2 = I64(M[1 * 3 + 1]) * V[1] + I64(M[1 * 3 + 2]) * V[2];
			MAC3 = I64(M[2 * 3 + 1]) * V[1] + I64(M[2 * 3 + 2]) * V[2];

			overflowCheckMAC(1, MAC1, mCurrentCommand.ShiftFraction);
			overflowCheckMAC(2, MAC2, mCurrentCommand.ShiftFraction);
			overflowCheckMAC(3, MAC3, mCurrentCommand.ShiftFraction);

			MAC1 = I64(T[0]) * 0x1000 + I64(M[0 * 3 + 0]) * V[0];
			MAC2 = I64(T[1]) * 0x1000 + I64(M[1 * 3 + 0]) * V[0];
			MAC3 = I64(T[2]) * 0x1000 + I64(M[2 * 3 + 0]) * V[0];

			overflowCheckMAC(1, MAC1, mCurrentCommand.ShiftFraction, ESX_FALSE);
			overflowCheckMAC(2, MAC2, mCurrentCommand.ShiftFraction, ESX_FALSE);
			overflowCheckMAC(3, MAC3, mCurrentCommand.ShiftFraction, ESX_FALSE);

			MAC1 >>= mCurrentCommand.ShiftFraction * 12;
			MAC2 >>= mCurrentCommand.ShiftFraction * 12;
			MAC3 >>= mCurrentCommand.ShiftFraction * 12;

			overfowCheckIR(1, MAC1, ESX_FALSE, ESX_FALSE);
			overfowCheckIR(2, MAC2, ESX_FALSE, ESX_FALSE);
			overfowCheckIR(3, MAC3, ESX_FALSE, ESX_FALSE);
		} else {
			Multiply(T, V, M, MAC1, MAC2, MAC3, mCurrentCommand.ShiftFraction);
		}

		overfowCheckIR(1, mRegisters.MACV[0], mCurrentCommand.Saturate);
		overfowCheckIR(2, mRegisters.MACV[1], mCurrentCommand.Saturate);
		overfowCheckIR(3, mRegisters.MACV[2], mCurrentCommand.Saturate);
	}

	void GTE::NCDS() {
		Internal_NCDS(mRegisters.V0);
	}

	void GTE::CDP() {
		I64 MAC1, MAC2, MAC3;

		/*[IR1,IR2,IR3] = [MAC1,MAC2,MAC3] = (BK*1000h + LCM*IR) SAR (sf*12)*/
		Array<I32, 3> IR = { mRegisters.IR1, mRegisters.IR2, mRegisters.IR3 };
		Multiply(mRegisters.BK, IR, mRegisters.LCM, MAC1, MAC2, MAC3, mCurrentCommand.ShiftFraction);

		overfowCheckIR(1, mRegisters.MACV[0], mCurrentCommand.Saturate);
		overfowCheckIR(2, mRegisters.MACV[1], mCurrentCommand.Saturate);
		overfowCheckIR(3, mRegisters.MACV[2], mCurrentCommand.Saturate);

		/*[MAC1,MAC2,MAC3] = [R*IR1,G*IR2,B*IR3] SHL 4 */
		I64 MAC1_orig = (I64(mRegisters.RGBC[0]) << 4) * I64(mRegisters.IR1);
		I64 MAC2_orig = (I64(mRegisters.RGBC[1]) << 4) * I64(mRegisters.IR2);
		I64 MAC3_orig = (I64(mRegisters.RGBC[2]) << 4) * I64(mRegisters.IR3);

		/*[IR1,IR2,IR3] = (([RFC,GFC,BFC] SHL 12) - [MAC1,MAC2,MAC3]) SAR (sf*12)*/
		MAC1 = (I64(mRegisters.FC[0]) << 12) - MAC1_orig;
		MAC2 = (I64(mRegisters.FC[1]) << 12) - MAC2_orig;
		MAC3 = (I64(mRegisters.FC[2]) << 12) - MAC3_orig;

		overflowCheckMAC(1, MAC1, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(2, MAC2, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(3, MAC3, mCurrentCommand.ShiftFraction);

		overfowCheckIR(1, mRegisters.MACV[0], ESX_FALSE);
		overfowCheckIR(2, mRegisters.MACV[1], ESX_FALSE);
		overfowCheckIR(3, mRegisters.MACV[2], ESX_FALSE);

		/*[MAC1,MAC2,MAC3] = (([IR1,IR2,IR3] * IR0) + [MAC1,MAC2,MAC3])*/
		/*[MAC1,MAC2,MAC3] = [MAC1,MAC2,MAC3] SAR (sf*12)*/
		MAC1 = MAC1_orig + (I64(mRegisters.IR0) * I64(mRegisters.IR1));
		MAC2 = MAC2_orig + (I64(mRegisters.IR0) * I64(mRegisters.IR2));
		MAC3 = MAC3_orig + (I64(mRegisters.IR0) * I64(mRegisters.IR3));

		overflowCheckMAC(1, MAC1, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(2, MAC2, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(3, MAC3, mCurrentCommand.ShiftFraction);

		/*Color FIFO = [MAC1/16,MAC2/16,MAC3/16,CODE]*/
		pushColor(mRegisters.MACV[0] >> 4, mRegisters.MACV[1] >> 4, mRegisters.MACV[2] >> 4);

		/*[IR1,IR2,IR3] = [MAC1,MAC2,MAC3]*/
		overfowCheckIR(1, mRegisters.MACV[0], mCurrentCommand.Saturate);
		overfowCheckIR(2, mRegisters.MACV[1], mCurrentCommand.Saturate);
		overfowCheckIR(3, mRegisters.MACV[2], mCurrentCommand.Saturate);
	}

	void GTE::NCDT() {
		Internal_NCDS(mRegisters.V0);
		Internal_NCDS(mRegisters.V1);
		Internal_NCDS(mRegisters.V2);
	}

	void GTE::NCCS() {
		Internal_NCCS(mRegisters.V0);
	}

	void GTE::CC() {
		I64 MAC1, MAC2, MAC3;

		/*[IR1,IR2,IR3] = [MAC1,MAC2,MAC3] = (BK*1000h + LCM*IR) SAR (sf*12)*/
		Array<I32, 3> IR = { mRegisters.IR1, mRegisters.IR2, mRegisters.IR3 };
		Multiply(mRegisters.BK, IR, mRegisters.LCM, MAC1, MAC2, MAC3, mCurrentCommand.ShiftFraction);

		overfowCheckIR(1, mRegisters.MACV[0], mCurrentCommand.Saturate);
		overfowCheckIR(2, mRegisters.MACV[1], mCurrentCommand.Saturate);
		overfowCheckIR(3, mRegisters.MACV[2], mCurrentCommand.Saturate);

		/*[MAC1,MAC2,MAC3] = [R*IR1,G*IR2,B*IR3] SHL 4 */
		I64 MAC1_orig = (I64(mRegisters.RGBC[0]) << 4) * I64(mRegisters.IR1);
		I64 MAC2_orig = (I64(mRegisters.RGBC[1]) << 4) * I64(mRegisters.IR2);
		I64 MAC3_orig = (I64(mRegisters.RGBC[2]) << 4) * I64(mRegisters.IR3);

		overflowCheckMAC(1, MAC1_orig, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(2, MAC2_orig, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(3, MAC3_orig, mCurrentCommand.ShiftFraction);

		/*Color FIFO = [MAC1/16,MAC2/16,MAC3/16,CODE]*/
		pushColor(mRegisters.MACV[0] >> 4, mRegisters.MACV[1] >> 4, mRegisters.MACV[2] >> 4);

		/*[IR1,IR2,IR3] = [MAC1,MAC2,MAC3]*/
		overfowCheckIR(1, mRegisters.MACV[0], mCurrentCommand.Saturate);
		overfowCheckIR(2, mRegisters.MACV[1], mCurrentCommand.Saturate);
		overfowCheckIR(3, mRegisters.MACV[2], mCurrentCommand.Saturate);
	}

	void GTE::NCS() {
		Internal_NCS(mRegisters.V0);
	}

	void GTE::NCT() {
		Internal_NCS(mRegisters.V0);
		Internal_NCS(mRegisters.V1);
		Internal_NCS(mRegisters.V2);
	}

	void GTE::SQR() {
		I64 MAC1 = I64(mRegisters.IR1) * I64(mRegisters.IR1);
		I64 MAC2 = I64(mRegisters.IR2) * I64(mRegisters.IR2);
		I64 MAC3 = I64(mRegisters.IR3) * I64(mRegisters.IR3);

		overflowCheckMAC(1, MAC1, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(2, MAC2, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(3, MAC3, mCurrentCommand.ShiftFraction);

		overfowCheckIR(1, mRegisters.MACV[0], mCurrentCommand.Saturate);
		overfowCheckIR(2, mRegisters.MACV[1], mCurrentCommand.Saturate);
		overfowCheckIR(3, mRegisters.MACV[2], mCurrentCommand.Saturate);
	}

	void GTE::DCPL() {
		/*[MAC1,MAC2,MAC3] = [R*IR1,G*IR2,B*IR3] SHL 4 */
		I64 MAC1_orig = (I64(mRegisters.RGBC[0]) << 4) * I64(mRegisters.IR1);
		I64 MAC2_orig = (I64(mRegisters.RGBC[1]) << 4) * I64(mRegisters.IR2);
		I64 MAC3_orig = (I64(mRegisters.RGBC[2]) << 4) * I64(mRegisters.IR3);

		/*[MAC1,MAC2,MAC((U64)mRegisters.IR2 << 12)3] = [IR1,IR2,IR3] SHL 12 */
		/*[IR1,IR2,IR3] ((U64)mRegisters.IR3 << 12)= (([RFC,GFC,BFC] SHL 12) - [MAC1,MAC2,MAC3]) SAR (sf*12)*/
		I64 MAC1 = ((I64)mRegisters.FC[0] << 12) - MAC1_orig;
		I64 MAC2 = ((I64)mRegisters.FC[1] << 12) - MAC2_orig;
		I64 MAC3 = ((I64)mRegisters.FC[2] << 12) - MAC3_orig;

		overflowCheckMAC(1, MAC1, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(2, MAC2, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(3, MAC3, mCurrentCommand.ShiftFraction);

		overfowCheckIR(1, mRegisters.MACV[0], ESX_FALSE);
		overfowCheckIR(2, mRegisters.MACV[1], ESX_FALSE);
		overfowCheckIR(3, mRegisters.MACV[2], ESX_FALSE);

		/*[MAC1,MAC2,MAC3] = (([IR1,IR2,IR3] * IR0) + [MAC1,MAC2,MAC3])*/
		MAC1 = (mRegisters.IR1 * mRegisters.IR0) + MAC1_orig;
		MAC2 = (mRegisters.IR2 * mRegisters.IR0) + MAC2_orig;
		MAC3 = (mRegisters.IR3 * mRegisters.IR0) + MAC3_orig;

		overflowCheckMAC(1, MAC1, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(2, MAC2, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(3, MAC3, mCurrentCommand.ShiftFraction);

		/*Color FIFO = [MAC1/16,MAC2/16,MAC3/16,CODE]*/
		pushColor(mRegisters.MACV[0] >> 4, mRegisters.MACV[1] >> 4, mRegisters.MACV[2] >> 4);

		/*[IR1,IR2,IR3] = [MAC1,MAC2,MAC3]*/
		overfowCheckIR(1, mRegisters.MACV[0], mCurrentCommand.Saturate);
		overfowCheckIR(2, mRegisters.MACV[1], mCurrentCommand.Saturate);
		overfowCheckIR(3, mRegisters.MACV[2], mCurrentCommand.Saturate);
	}

	void GTE::DPCT() {
		Internal_DPCS(mRegisters.RGB0);
		Internal_DPCS(mRegisters.RGB0);
		Internal_DPCS(mRegisters.RGB0);
	}

	void GTE::AVSZ3() {
		I64 MAC0 = I64(mRegisters.ZSF3) * (I64(mRegisters.SZ1) + mRegisters.SZ2 + mRegisters.SZ3);
		I64 OTZ = MAC0 / 0x1000;

		overflowCheckMAC(0, MAC0, ESX_FALSE);

		if (OTZ < 0 || OTZ > 0xFFFF) {
			OTZ = std::clamp<I64>(OTZ, 0x0000, 0xFFFF);
			mRegisters.FLAG |= FlagRegisterSZ3OTZSat;
		}

		mRegisters.OTZ = static_cast<U16>(OTZ);
	}

	void GTE::AVSZ4() {
		I64 MAC0 = I64(mRegisters.ZSF4) * (I64(mRegisters.SZ0) + I64(mRegisters.SZ1) + mRegisters.SZ2 + mRegisters.SZ3);
		I64 OTZ = MAC0 / 0x1000;

		overflowCheckMAC(0, MAC0, ESX_FALSE);

		if (OTZ < 0 || OTZ > 0xFFFF) {
			OTZ = std::clamp<I64>(OTZ, 0x0000, 0xFFFF);
			mRegisters.FLAG |= FlagRegisterSZ3OTZSat;
		}

		mRegisters.OTZ = static_cast<U16>(OTZ);
	}

	void GTE::RTPT() {
		I32 H_CALC = 0;

		Internal_RTPS(mRegisters.V0, mCurrentCommand.Saturate, mCurrentCommand.ShiftFraction, H_CALC);
		Internal_RTPS(mRegisters.V1, mCurrentCommand.Saturate, mCurrentCommand.ShiftFraction, H_CALC);
		Internal_RTPS(mRegisters.V2, mCurrentCommand.Saturate, mCurrentCommand.ShiftFraction, H_CALC);

		I64 MAC0 = H_CALC * I64(mRegisters.DQA) + I64(mRegisters.DQB);
		overflowCheckMAC(0, MAC0, ESX_FALSE);

		I64 MAC012 = MAC0 >> 12;
		if (MAC012 < 0x0000 || MAC012 > 0x1000) mRegisters.FLAG |= FlagRegisterIR0Sat;
		mRegisters.IR0 = std::clamp<I32>(MAC012, 0x0000, 0x1000);
	}

	void GTE::GPF() {
		I64 MAC1, MAC2, MAC3;

		/*[MAC1,MAC2,MAC3] = 0*/
		/*[MAC1,MAC2,MAC3] = (([IR1,IR2,IR3] * IR0) + [MAC1,MAC2,MAC3]) SAR (sf*12)*/
		MAC1 = (I64(mRegisters.IR1) * mRegisters.IR0);
		MAC2 = (I64(mRegisters.IR2) * mRegisters.IR0);
		MAC3 = (I64(mRegisters.IR3) * mRegisters.IR0);

		overflowCheckMAC(1, MAC1, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(2, MAC2, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(3, MAC3, mCurrentCommand.ShiftFraction);

		/*Color FIFO = [MAC1/16,MAC2/16,MAC3/16,CODE]*/
		pushColor(mRegisters.MACV[0] >> 4, mRegisters.MACV[1] >> 4, mRegisters.MACV[2] >> 4);

		/*[IR1,IR2,IR3] = [MAC1,MAC2,MAC3]*/
		overfowCheckIR(1, mRegisters.MACV[0], mCurrentCommand.Saturate);
		overfowCheckIR(2, mRegisters.MACV[1], mCurrentCommand.Saturate);
		overfowCheckIR(3, mRegisters.MACV[2], mCurrentCommand.Saturate);
	}

	void GTE::GPL() {
		I64 MAC1, MAC2, MAC3;

		/*[MAC1,MAC2,MAC3] = [MAC1,MAC2,MAC3] SHL (sf*12)*/
		MAC1 = (I64)mRegisters.MACV[0] << (mCurrentCommand.ShiftFraction * 12);
		MAC2 = (I64)mRegisters.MACV[1] << (mCurrentCommand.ShiftFraction * 12);
		MAC3 = (I64)mRegisters.MACV[2] << (mCurrentCommand.ShiftFraction * 12);

		/*[MAC1,MAC2,MAC3] = (([IR1,IR2,IR3] * IR0) + [MAC1,MAC2,MAC3]) SAR (sf*12)*/
		MAC1 = (I64(mRegisters.IR1) * mRegisters.IR0) + MAC1;
		MAC2 = (I64(mRegisters.IR2) * mRegisters.IR0) + MAC2;
		MAC3 = (I64(mRegisters.IR3) * mRegisters.IR0) + MAC3;

		overflowCheckMAC(1, MAC1, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(2, MAC2, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(3, MAC3, mCurrentCommand.ShiftFraction);

		/*Color FIFO = [MAC1/16,MAC2/16,MAC3/16,CODE]*/
		pushColor(mRegisters.MACV[0] >> 4, mRegisters.MACV[1] >> 4, mRegisters.MACV[2] >> 4);

		/*[IR1,IR2,IR3] = [MAC1,MAC2,MAC3]*/
		overfowCheckIR(1, mRegisters.MACV[0], mCurrentCommand.Saturate);
		overfowCheckIR(2, mRegisters.MACV[1], mCurrentCommand.Saturate);
		overfowCheckIR(3, mRegisters.MACV[2], mCurrentCommand.Saturate);
	}

	void GTE::NCCT() {
		Internal_NCCS(mRegisters.V0);
		Internal_NCCS(mRegisters.V1);
		Internal_NCCS(mRegisters.V2);
	}

	void GTE::NA() {
		ESX_CORE_LOG_ERROR("GTE::NA Called");
	}

	static const Array<Pair<GTEExecuteFunction, U32>, 0x40> gteOpCodeDecode = {
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::RTPS, 15),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NCLIP, 8),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::OP, 6),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::DPCS, 8),
		std::make_pair(&GTE::INTPL, 8),
		std::make_pair(&GTE::MVMVA, 8),
		std::make_pair(&GTE::NCDS, 19),
		std::make_pair(&GTE::CDP, 13),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NCDT, 44),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NCCS, 17),
		std::make_pair(&GTE::CC, 11),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NCS, 14),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NCT, 30),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::SQR, 5),
		std::make_pair(&GTE::DCPL, 8),
		std::make_pair(&GTE::DPCT, 17),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::AVSZ3, 5),
		std::make_pair(&GTE::AVSZ4, 6),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::RTPT, 23),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::NA, 0),
		std::make_pair(&GTE::GPF, 5),
		std::make_pair(&GTE::GPL, 5),
		std::make_pair(&GTE::NCCT, 39)
	};

	GTECommand GTE::decodeCommand(U32 value) {
		GTECommand command = {};

		command.RealCommandNumber = (value >> 0) & 0x3F;
		command.Saturate = (value >> 10) & 0x1;
		command.TranslationVector = (TranslationVector)((value >> 13) & 0x3);
		command.MultiplyVector = (MultiplyVector)((value >> 15) & 0x3);
		command.MultiplyMatrix = (MultiplyMatrix)((value >> 17) & 0x3);
		command.ShiftFraction = (value >> 19) & 0x1;
		command.FakeCommandNumber = (value >> 20) & 0x1F;

		auto& pair = gteOpCodeDecode[command.RealCommandNumber];

		command.Execute = pair.first;
		command.TotalClocks = pair.second;

		return command;
	}

	void GTE::Internal_RTPS(const Array<I16, 3>& V, BIT lm, BIT sf, I32& H_CALC)
	{
		Array<I64, 2> SXY2 = {};
		I64 MAC0, MAC1, MAC2, MAC3;

		Multiply(mRegisters.TR, V, mRegisters.RT, MAC1, MAC2, MAC3, sf);

		overfowCheckIR(1, mRegisters.MACV[0], lm);
		overfowCheckIR(2, mRegisters.MACV[1], lm);

		I64 MAC3sf = MAC3 >> (sf * 12);
		I64 MAC312 = MAC3 >> 12;
		I64 minValue = lm ? 0x0000 : -0x8000;
		I64 maxValue = 0x7FFF;
		if(MAC312 < -0x8000 || MAC312 > 0x7FFF) mRegisters.FLAG |= FlagRegisterIR3Sat;
		mRegisters.IR3 = std::clamp<I32>(MAC3sf, minValue, maxValue);

		I32 SZ3 = MAC312;
		pushSZ(SZ3);

		H_CALC = 0;
		if (mRegisters.H < (mRegisters.SZ3 * 2)) {
			I32 shift = std::countl_zero<U16>(mRegisters.SZ3);
			I32 r1 = (mRegisters.SZ3 << shift) & 0x7fff;
			I32 r2 = unr_table[((r1 + 0x40) >> 7)] + 0x101;
			I32 r3 = ((0x80 - (r2 * (r1 + 0x8000))) >> 8) & 0x1ffff;
			U32 reciprocal = ((r2 * r3) + 0x80) >> 8;
			U32 res = ((((U64)reciprocal * (mRegisters.H << shift)) + 0x8000) >> 16);
			H_CALC = std::min<uint32_t>(0x1FFFF, res);
		} else {
			H_CALC = 0x1FFFF;
			mRegisters.FLAG |= FlagRegisterDivOver;
		}

		MAC0 = H_CALC * I64(mRegisters.IR1) + I64(mRegisters.OF[0]); 
		overflowCheckMAC(0, MAC0, ESX_FALSE, ESX_FALSE);
		SXY2[0] = MAC0 >> 16;

		MAC0 = H_CALC * I64(mRegisters.IR2) + I64(mRegisters.OF[1]);
		overflowCheckMAC(0, MAC0, ESX_FALSE, ESX_FALSE);
		SXY2[1] = MAC0 >> 16;

		pushSXY(SXY2);
	}

	void GTE::Internal_NCDS(const Array<I16, 3>& V)
	{
		I64 MAC1, MAC2, MAC3;

		/*[IR1,IR2,IR3] = [MAC1,MAC2,MAC3] = (LLM*V0) SAR (sf*12)*/
		Multiply(V, mRegisters.LLM, MAC1, MAC2, MAC3, mCurrentCommand.ShiftFraction);

		overfowCheckIR(1, mRegisters.MACV[0], mCurrentCommand.Saturate);
		overfowCheckIR(2, mRegisters.MACV[1], mCurrentCommand.Saturate);
		overfowCheckIR(3, mRegisters.MACV[2], mCurrentCommand.Saturate);

		/*[IR1,IR2,IR3] = [MAC1,MAC2,MAC3] = (BK*1000h + LCM*IR) SAR (sf*12)*/
		Array<I32, 3> IR = { mRegisters.IR1, mRegisters.IR2, mRegisters.IR3 };
		Multiply(mRegisters.BK, IR, mRegisters.LCM, MAC1, MAC2, MAC3, mCurrentCommand.ShiftFraction);

		overfowCheckIR(1, mRegisters.MACV[0], mCurrentCommand.Saturate);
		overfowCheckIR(2, mRegisters.MACV[1], mCurrentCommand.Saturate);
		overfowCheckIR(3, mRegisters.MACV[2], mCurrentCommand.Saturate);

		/*[MAC1,MAC2,MAC3] = [R*IR1,G*IR2,B*IR3] SHL 4 */
		I64 MAC1_orig = (I64(mRegisters.RGBC[0]) << 4) * I64(mRegisters.IR1);
		I64 MAC2_orig = (I64(mRegisters.RGBC[1]) << 4) * I64(mRegisters.IR2);
		I64 MAC3_orig = (I64(mRegisters.RGBC[2]) << 4) * I64(mRegisters.IR3);

		/*[IR1,IR2,IR3] = (([RFC,GFC,BFC] SHL 12) - [MAC1,MAC2,MAC3]) SAR (sf*12)*/
		MAC1 = (I64(mRegisters.FC[0]) << 12) - MAC1_orig;
		MAC2 = (I64(mRegisters.FC[1]) << 12) - MAC2_orig;
		MAC3 = (I64(mRegisters.FC[2]) << 12) - MAC3_orig;

		overflowCheckMAC(1, MAC1, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(2, MAC2, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(3, MAC3, mCurrentCommand.ShiftFraction);

		overfowCheckIR(1, mRegisters.MACV[0], ESX_FALSE);
		overfowCheckIR(2, mRegisters.MACV[1], ESX_FALSE);
		overfowCheckIR(3, mRegisters.MACV[2], ESX_FALSE);

		/*[MAC1,MAC2,MAC3] = (([IR1,IR2,IR3] * IR0) + [MAC1,MAC2,MAC3])*/
		/*[MAC1,MAC2,MAC3] = [MAC1,MAC2,MAC3] SAR (sf*12)*/
		MAC1 = MAC1_orig + (I64(mRegisters.IR0) * I64(mRegisters.IR1));
		MAC2 = MAC2_orig + (I64(mRegisters.IR0) * I64(mRegisters.IR2));
		MAC3 = MAC3_orig + (I64(mRegisters.IR0) * I64(mRegisters.IR3));

		overflowCheckMAC(1, MAC1, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(2, MAC2, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(3, MAC3, mCurrentCommand.ShiftFraction);

		/*Color FIFO = [MAC1/16,MAC2/16,MAC3/16,CODE]*/
		pushColor(mRegisters.MACV[0] >> 4, mRegisters.MACV[1] >> 4, mRegisters.MACV[2] >> 4);

		/*[IR1,IR2,IR3] = [MAC1,MAC2,MAC3]*/
		overfowCheckIR(1, mRegisters.MACV[0], mCurrentCommand.Saturate);
		overfowCheckIR(2, mRegisters.MACV[1], mCurrentCommand.Saturate);
		overfowCheckIR(3, mRegisters.MACV[2], mCurrentCommand.Saturate);
	}

	void GTE::Internal_NCCS(const Array<I16, 3>& V)
	{
		I64 MAC1, MAC2, MAC3;

		/*[IR1,IR2,IR3] = [MAC1,MAC2,MAC3] = (LLM*V0) SAR (sf*12)*/
		Multiply(V, mRegisters.LLM, MAC1, MAC2, MAC3, mCurrentCommand.ShiftFraction);

		overfowCheckIR(1, mRegisters.MACV[0], mCurrentCommand.Saturate);
		overfowCheckIR(2, mRegisters.MACV[1], mCurrentCommand.Saturate);
		overfowCheckIR(3, mRegisters.MACV[2], mCurrentCommand.Saturate);

		/*[IR1,IR2,IR3] = [MAC1,MAC2,MAC3] = (BK*1000h + LCM*IR) SAR (sf*12)*/
		Array<I32, 3> IR = { mRegisters.IR1, mRegisters.IR2, mRegisters.IR3 };
		Multiply(mRegisters.BK, IR, mRegisters.LCM, MAC1, MAC2, MAC3, mCurrentCommand.ShiftFraction);

		overfowCheckIR(1, mRegisters.MACV[0], mCurrentCommand.Saturate);
		overfowCheckIR(2, mRegisters.MACV[1], mCurrentCommand.Saturate);
		overfowCheckIR(3, mRegisters.MACV[2], mCurrentCommand.Saturate);

		/*[MAC1,MAC2,MAC3] = [R*IR1,G*IR2,B*IR3] SHL 4 */
		I64 MAC1_orig = (I64(mRegisters.RGBC[0]) << 4) * I64(mRegisters.IR1);
		I64 MAC2_orig = (I64(mRegisters.RGBC[1]) << 4) * I64(mRegisters.IR2);
		I64 MAC3_orig = (I64(mRegisters.RGBC[2]) << 4) * I64(mRegisters.IR3);

		overflowCheckMAC(1, MAC1_orig, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(2, MAC2_orig, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(3, MAC3_orig, mCurrentCommand.ShiftFraction);

		/*Color FIFO = [MAC1/16,MAC2/16,MAC3/16,CODE]*/
		pushColor(mRegisters.MACV[0] >> 4, mRegisters.MACV[1] >> 4, mRegisters.MACV[2] >> 4);

		/*[IR1,IR2,IR3] = [MAC1,MAC2,MAC3]*/
		overfowCheckIR(1, mRegisters.MACV[0], mCurrentCommand.Saturate);
		overfowCheckIR(2, mRegisters.MACV[1], mCurrentCommand.Saturate);
		overfowCheckIR(3, mRegisters.MACV[2], mCurrentCommand.Saturate);
	}

	void GTE::Internal_NCS(const Array<I16, 3>& V)
	{
		I64 MAC1, MAC2, MAC3;

		/*[IR1,IR2,IR3] = [MAC1,MAC2,MAC3] = (LLM*V0) SAR (sf*12)*/
		Multiply(V, mRegisters.LLM, MAC1, MAC2, MAC3, mCurrentCommand.ShiftFraction);

		overfowCheckIR(1, mRegisters.MACV[0], mCurrentCommand.Saturate);
		overfowCheckIR(2, mRegisters.MACV[1], mCurrentCommand.Saturate);
		overfowCheckIR(3, mRegisters.MACV[2], mCurrentCommand.Saturate);

		/*[IR1,IR2,IR3] = [MAC1,MAC2,MAC3] = (BK*1000h + LCM*IR) SAR (sf*12)*/
		Array<I32, 3> IR = { mRegisters.IR1, mRegisters.IR2, mRegisters.IR3 };
		Multiply(mRegisters.BK, IR, mRegisters.LCM, MAC1, MAC2, MAC3, mCurrentCommand.ShiftFraction);

		overfowCheckIR(1, mRegisters.MACV[0], mCurrentCommand.Saturate);
		overfowCheckIR(2, mRegisters.MACV[1], mCurrentCommand.Saturate);
		overfowCheckIR(3, mRegisters.MACV[2], mCurrentCommand.Saturate);

		/*Color FIFO = [MAC1/16,MAC2/16,MAC3/16,CODE]*/
		pushColor(mRegisters.MACV[0] >> 4, mRegisters.MACV[1] >> 4, mRegisters.MACV[2] >> 4);

		/*[IR1,IR2,IR3] = [MAC1,MAC2,MAC3]*/
		overfowCheckIR(1, mRegisters.MACV[0], mCurrentCommand.Saturate);
		overfowCheckIR(2, mRegisters.MACV[1], mCurrentCommand.Saturate);
		overfowCheckIR(3, mRegisters.MACV[2], mCurrentCommand.Saturate);
	}

	void GTE::Internal_DPCS(const Array<U8, 4>& RGBC)
	{
		/*[MAC1,MAC2,MAC3] = [R,G,B] SHL 16*/
		/*[IR1,IR2,IR3] = (([RFC,GFC,BFC] SHL 12) - [MAC1,MAC2,MAC3]) SAR (sf*12)*/
		I64 MAC1 = (I64(mRegisters.FC[0]) << 12) - (U64(RGBC[0]) << 16);
		I64 MAC2 = (I64(mRegisters.FC[1]) << 12) - (U64(RGBC[1]) << 16);
		I64 MAC3 = (I64(mRegisters.FC[2]) << 12) - (U64(RGBC[2]) << 16);

		overflowCheckMAC(1, MAC1, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(2, MAC2, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(3, MAC3, mCurrentCommand.ShiftFraction);

		overfowCheckIR(1, mRegisters.MACV[0], ESX_FALSE);
		overfowCheckIR(2, mRegisters.MACV[1], ESX_FALSE);
		overfowCheckIR(3, mRegisters.MACV[2], ESX_FALSE);

		/*[MAC1,MAC2,MAC3] = (([IR1,IR2,IR3] * IR0) + [MAC1,MAC2,MAC3])*/
		MAC1 = (mRegisters.IR1 * mRegisters.IR0) + (U64(RGBC[0]) << 16);
		MAC2 = (mRegisters.IR2 * mRegisters.IR0) + (U64(RGBC[1]) << 16);
		MAC3 = (mRegisters.IR3 * mRegisters.IR0) + (U64(RGBC[2]) << 16);

		overflowCheckMAC(1, MAC1, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(2, MAC2, mCurrentCommand.ShiftFraction);
		overflowCheckMAC(3, MAC3, mCurrentCommand.ShiftFraction);

		/*Color FIFO = [MAC1/16,MAC2/16,MAC3/16,CODE]*/
		pushColor(mRegisters.MACV[0] >> 4, mRegisters.MACV[1] >> 4, mRegisters.MACV[2] >> 4);

		/*[IR1,IR2,IR3] = [MAC1,MAC2,MAC3]*/
		overfowCheckIR(1, mRegisters.MACV[0], mCurrentCommand.Saturate);
		overfowCheckIR(2, mRegisters.MACV[1], mCurrentCommand.Saturate);
		overfowCheckIR(3, mRegisters.MACV[2], mCurrentCommand.Saturate);
	}


	void GTE::Multiply(const Array<I16, 3>& V, const Array<I16, 9>& M, I64& MAC1, I64& MAC2, I64& MAC3, BIT sf)
	{
		I44 RES1 = (I44(I64(M[0 * 3 + 0]) * V[0]) + I64(M[0 * 3 + 1]) * V[1] + I64(M[0 * 3 + 2]) * V[2]);
		I44 RES2 = (I44(I64(M[1 * 3 + 0]) * V[0]) + I64(M[1 * 3 + 1]) * V[1] + I64(M[1 * 3 + 2]) * V[2]);
		I44 RES3 = (I44(I64(M[2 * 3 + 0]) * V[0]) + I64(M[2 * 3 + 1]) * V[1] + I64(M[2 * 3 + 2]) * V[2]);

		MAC1 = RES1.value();
		MAC2 = RES2.value();
		MAC3 = RES3.value();

		overflowCheckMAC(1, RES1, sf);
		overflowCheckMAC(2, RES2, sf);
		overflowCheckMAC(3, RES3, sf);
	}

	void GTE::overflowCheckMAC(U8 index, I44 value, BIT sf, BIT set)
	{
		if (index == 0) {
			if (value.value() > I64(0x7fffffffll)) {
				mRegisters.FLAG |= FlagRegisterMAC0Pos;
			}

			if (value.value() < I64(-0x80000000ll)) {
				mRegisters.FLAG |= FlagRegisterMAC0Neg;
			}

			if(set) mRegisters.MAC0 = static_cast<I32>(value.value() >> (sf * 12));
		}else {
			if (value.positiveOverflow()) {
				switch (index) {
					case 1: mRegisters.FLAG |= FlagRegisterMAC1Pos; break;
					case 2: mRegisters.FLAG |= FlagRegisterMAC2Pos; break;
					case 3: mRegisters.FLAG |= FlagRegisterMAC3Pos; break;
				}
			}

			if (value.negativeOverflow()) {
				switch (index) {
					case 1: mRegisters.FLAG |= FlagRegisterMAC1Neg; break;
					case 2: mRegisters.FLAG |= FlagRegisterMAC2Neg; break;
					case 3: mRegisters.FLAG |= FlagRegisterMAC3Neg; break;
				}
			}
			if (set) mRegisters.MACV[index - 1] = static_cast<I32>(value.value() >> (sf * 12));
		}
	}

	void GTE::overfowCheckIR(U8 index, I64 value, BIT lm, BIT set)
	{
		I64 minValue = lm ? 0x0000 : -0x8000;
		I64 maxValue = 0x7FFF;

		if (value < minValue || value > maxValue) {
			value = std::clamp(value, minValue, maxValue);

			switch (index) {
				case 0: mRegisters.FLAG |= FlagRegisterIR0Sat; break;
				case 1: mRegisters.FLAG |= FlagRegisterIR1Sat; break;
				case 2: mRegisters.FLAG |= FlagRegisterIR2Sat; break;
				case 3: mRegisters.FLAG |= FlagRegisterIR3Sat; break;
			}
		}

		if (set) {
			switch (index) {
				case 0: mRegisters.IR0 = value; break;
				case 1: mRegisters.IR1 = value; break;
				case 2: mRegisters.IR2 = value; break;
				case 3: mRegisters.IR3 = value; break;
			}
		}
	}

	void GTE::pushSZ(I32 value)
	{
		if (value > 0xFFFF || value < 0x0000) {
			value = std::clamp<I32>(value, 0x0000, 0xFFFF);
			mRegisters.FLAG |= FlagRegisterSZ3OTZSat;
		}

		mRegisters.SZ0 = mRegisters.SZ1;
		mRegisters.SZ1 = mRegisters.SZ2;
		mRegisters.SZ2 = mRegisters.SZ3;
		mRegisters.SZ3 = static_cast<U16>(value);
	}

	void GTE::pushSXY(Array<I64, 2>& value)
	{
		if (value[0] < -0x400 || value[0] > 0x3FF) {
			value[0] = std::clamp<I64>(value[0], -0x400, 0x3FF);
			mRegisters.FLAG |= FlagRegisterSX2Sat;
		}

		if (value[1] < -0x400 || value[1] > 0x3FF) {
			value[1] = std::clamp<I64>(value[1], -0x400, 0x3FF);
			mRegisters.FLAG |= FlagRegisterSY2Sat;
		}

		mRegisters.SXY0 = mRegisters.SXY1;
		mRegisters.SXY1 = mRegisters.SXY2;
		mRegisters.SXY2[0] = value[0];
		mRegisters.SXY2[1] = value[1];
	}

	void GTE::pushColor(I32 r, I32 g, I32 b)
	{
		if (r < 0 || r > 0xFF) {
			r = std::clamp(r, 0x00, 0xFF);
			mRegisters.FLAG |= FlagRegisterColorFIFORSat;
		}

		if (g < 0 || g > 0xFF) {
			g = std::clamp(g, 0x00, 0xFF);
			mRegisters.FLAG |= FlagRegisterColorFIFOGSat;
		}

		if (b < 0 || b > 0xFF) {
			b = std::clamp(b, 0x00, 0xFF);
			mRegisters.FLAG |= FlagRegisterColorFIFOBSat;
		}

		U8 c = mRegisters.RGBC[3];

		mRegisters.RGB0 = mRegisters.RGB1;
		mRegisters.RGB1 = mRegisters.RGB2;
		mRegisters.Registers[22] = (r << 0) | (g << 8) | (b << 16) | (c << 24);
	}

}