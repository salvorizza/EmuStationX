#pragma once

#include "Base/Base.h"

#include "Utils/LoggingSystem.h"

namespace esx {

	enum class MultiplyMatrix {
		Rotation,
		Light,
		Color,
		Reserved
	};

	enum class MultiplyVector {
		V0,
		V1,
		V2,
		IR_Long
	};

	enum class TranslationVector {
		TR,
		BK,
		FC_Bugged,
		None
	};

	enum FlagRegister {
		FlagRegisterIR0Sat = 1 << 12,
		FlagRegisterSY2Sat = 1 << 13,
		FlagRegisterSX2Sat = 1 << 14,
		FlagRegisterMAC0Neg = 1 << 15,
		FlagRegisterMAC0Pos = 1 << 16,
		FlagRegisterDivOver = 1 << 17,
		FlagRegisterSZ3OTZSat = 1 << 18,
		FlagRegisterColorFIFOBSat = 1 << 19,
		FlagRegisterColorFIFOGSat = 1 << 20,
		FlagRegisterColorFIFORSat = 1 << 21,
		FlagRegisterIR3Sat= 1 << 22,
		FlagRegisterIR2Sat = 1 << 23,
		FlagRegisterIR1Sat = 1 << 24,
		FlagRegisterMAC3Neg = 1 << 25,
		FlagRegisterMAC2Neg = 1 << 26,
		FlagRegisterMAC1Neg = 1 << 27,
		FlagRegisterMAC3Pos = 1 << 28,
		FlagRegisterMAC2Pos = 1 << 29,
		FlagRegisterMAC1Pos = 1 << 30,
		FlagRegisterError = 1 << 31
	};

	class GTE;
	typedef void(GTE::* GTEExecuteFunction)();

	struct GTECommand {
		U8 FakeCommandNumber = 0x00;
		BIT ShiftFraction = ESX_FALSE;
		MultiplyMatrix MultiplyMatrix = MultiplyMatrix::Rotation;
		MultiplyVector MultiplyVector = MultiplyVector::V0;
		TranslationVector TranslationVector = TranslationVector::TR;
		BIT Saturate = ESX_FALSE;
		U8 RealCommandNumber = 0x00;
		GTEExecuteFunction Execute = nullptr;
		U32 TotalClocks = 0x00;
		U32 RemainingClocks = 0x00;
	};

	union GTERegisters {
		Array<U32, 64> Registers = {};

		struct {
			Array<I16, 3> V0;
			U16 PAD1;
			Array<I16, 3> V1;
			U16 PAD2;
			Array<I16, 3> V2;
			U16 PAD3;
			Array<U8, 4> RGBC;
			U16 OTZ;
			U16 PAD4;
			I32 IR0;
			I32 IR1;
			I32 IR2;
			I32 IR3;
			Array<I16, 2> SXY0;
			Array<I16, 2> SXY1;
			Array<I16, 2> SXY2;
			Array<I16, 2> SXYP;
			U16 SZ0;
			U16 PAD9;
			U16 SZ1;
			U16 PAD10;
			U16 SZ2;
			U16 PAD11;
			U16 SZ3;
			U16 PAD12;
			Array<U8, 4> RGB0;
			Array<U8, 4> RGB1;
			Array<U8, 4> RGB2;
			U32 RES1;
			I32 MAC0;
			Array<I32, 3> MACV;
			U32 IRGB;
			U32 ORGB;
			I32 LZCS;
			I32 LZCR;
			Array<I16, 9> RT;
			U16 PAD13;
			Array<I32, 3> TR;
			Array<I16, 9> LLM;
			U16 PAD14;
			Array<I32, 3> BK;
			Array<I16, 9> LCM;
			U16 PAD15;
			Array<I32, 3> FC;
			Array<I32, 2> OF;
			U16 H;
			U16 PAD16;
			I16 DQA;
			U16 PAD17;
			I32 DQB;
			I16 ZSF3;
			U16 PAD18;
			I16 ZSF4;
			U16 PAD19;
			U32 FLAG;
		};
	};
	static_assert(sizeof(GTERegisters) == (sizeof(U32) * 64));

	class I44 {
	public:
		I44(I64 value)
			: m_value(value), m_positive_overflow(value > 0x7ffffffffff), m_negative_overflow(value < -0x80000000000) {}

		I44(I64 value, bool positive_overflow, bool negative_overflow)
			: m_value(value), m_positive_overflow(positive_overflow), m_negative_overflow(negative_overflow) {}

		I44 operator+(I64 rhs) {
			I64 value = ((m_value + rhs) << 20) >> 20;
			return I44(value, m_positive_overflow || (value < 0 && m_value >= 0 && rhs >= 0),
				m_negative_overflow || (value >= 0 && m_value < 0 && rhs < 0));
		}

		bool positiveOverflow() { return m_positive_overflow; }
		bool negativeOverflow() { return m_negative_overflow; }
		I64 value() { return m_value; }

	private:
		I64 m_value;
		BIT m_positive_overflow;
		BIT m_negative_overflow;
	};

	class GTE {
		friend class CPUStatusPanel;
	public:
		GTE() = default;
		~GTE() = default;

		void clock();

		void command(U32 command);

		U32 getRegister(U32 index);
		void setRegister(U32 index, U32 value);

		BIT getFlag() { return false; }

		void reset();

		void RTPS();
		void NCLIP();
		void OP();
		void DPCS();
		void INTPL();
		void MVMVA();
		void NCDS();
		void CDP();
		void NCDT();
		void NCCS();
		void CC();
		void NCS();
		void NCT();
		void SQR();
		void DCPL();
		void DPCT();
		void AVSZ3();
		void AVSZ4();
		void RTPT();
		void GPF();
		void GPL();
		void NCCT();
		void NA();

	private:
		GTECommand decodeCommand(U32 value);

		void Internal_RTPS(const Array<I16, 3>& V, BIT lm, BIT sf, I32& H_CALC);
		void Internal_NCDS(const Array<I16, 3>& V);
		void Internal_NCCS(const Array<I16, 3>& V);
		void Internal_NCS(const Array<I16, 3>& V);
		void Internal_DPCS(const Array<U8, 4>& RGBC);

		template<typename TVec>
		void Multiply(const Array<I32, 3>& T, const Array<TVec, 3>& V, const Array<I16, 9>& M, I64& MAC1, I64& MAC2, I64& MAC3, BIT sf) {
			I44 RES1 = (I44(I64(T[0]) << 12) + I64(M[0 * 3 + 0]) * V[0] + I64(M[0 * 3 + 1]) * V[1] + I64(M[0 * 3 + 2]) * V[2]);
			I44 RES2 = (I44(I64(T[1]) << 12) + I64(M[1 * 3 + 0]) * V[0] + I64(M[1 * 3 + 1]) * V[1] + I64(M[1 * 3 + 2]) * V[2]);
			I44 RES3 = (I44(I64(T[2]) << 12) + I64(M[2 * 3 + 0]) * V[0] + I64(M[2 * 3 + 1]) * V[1] + I64(M[2 * 3 + 2]) * V[2]);

			MAC1 = RES1.value();
			MAC2 = RES2.value();
			MAC3 = RES3.value();

			overflowCheckMAC(1, RES1, sf);
			overflowCheckMAC(2, RES2, sf);
			overflowCheckMAC(3, RES3, sf);	
		}
		void Multiply(const Array<I16, 3>& V, const Array<I16, 9>& M, I64& MAC1, I64& MAC2, I64& MAC3, BIT sf);

		void overflowCheckMAC(U8 index, I44 value, BIT sf, BIT set = ESX_TRUE);
		void overfowCheckIR(U8 index, I64 value, BIT lm, BIT set = ESX_TRUE);
		void pushSZ(I32 value);
		void pushSXY(Array<I32,2>& value);
		void pushColor(I32 r, I32 g, I32 b);

	private:
		GTERegisters mRegisters = {};
		GTECommand mCurrentCommand = {};
	};

}