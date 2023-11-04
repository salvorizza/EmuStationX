#pragma once

#include <cstdint>
#include <array>
#include <string>
#include <functional>
#include <format>
#include <queue>

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {

	#define OPCODE(x) (x >> 26) & 0x3F
	#define RS(x) (x >> 21) & 0x1F
	#define RT(x) (x >> 16) & 0x1F
	#define RD(x) (x >> 11) & 0x1F
	#define SHAMT(x) (x >> 6) & 0x1F
	#define FUNCT(x) x & 0x3F
	#define IMM(x) x & 0xFFFF
	#define ADDRESS(x) x & 0x3FFFFFF
	#define CODE(x) (x >> 6) & 0xFFFFF
	#define CO(x) ((x >> 25) & 0x1)
	#define CO_N(x) ((x >> 26) & 0x3)
	#define COP_FUNC(x) (x & 0x1F)


	#define SIGNEXT8(x) ((x & 0x80) ? x | 0xFFFFFF00 : x)
	#define SIGNEXT16(x) ((x & 0x8000) ? x | 0xFFFF0000 : x)
	#define EXCEPTION_HANDLER_ADDRESS 0x80000080
	#define BREAKPOINT_EXCEPTION_HANDLER_ADDRESS 0x80000040
	#define ADDRESS_UNALIGNED(x,type) ((x & (sizeof(type) - 1)) != 0x0)
	#define OVERFLOW_ADD32(a,b,s) (~((a & 0x80000000) ^ (b & 0x80000000)) & ((a & 0x80000000) ^ (s & 0x80000000)))
	#define OVERFLOW_SUB32(a,b,s) ((a & 0x80000000) ^ (b & 0x80000000)) & ((a & 0x80000000) ^ (s & 0x80000000))

	constexpr std::array<U32, 8> SEGS_MASKS = {
		//KUSEG:2048MB
		0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
		//KSEG0:512MB
		0x7FFFFFFF,
		//KSEG1:512MB
		0x1FFFFFFF,
		//KSEG2:1024MB
		0xFFFFFFFF,0xFFFFFFFF
	};

	enum class COP0Register : U8 {
		BPC = 3, //Breakpoint on Execute Address (R/W)
		BDA = 5, //Breakpoint on Data Access Address (R/W)
		JumpDest = 6, //Randomly memorized jump address
		DCIC = 7, //Breakpoint control (R/W)
		BadVAddr = 8, //Stores virtual address for the most recent address related exception
		BDAM = 9, //Breakpoint on Data Access Mask (R/W)
		BPCM = 11, //Breakpoint on Execute Mask (R/W)
		SR = 12, //Process status register/Used for exception handling
		Cause = 13, //Exception cause register/Stores the type of exception that last occurred
		EPC = 14, //(ExceptionPC) Contains address of instruction that caused the exception
		PRId = 15 //Processor identification and revision.
	};

	enum class ExceptionType : U8 {
		Interrupt = 0x00,
		AddressErrorLoad = 0x04,
		AddressErrorStore = 0x05,
		Syscall = 0x08,
		Breakpoint = 0x09,
		ReservedInstruction = 0x0A,
		CoprocessorUnusable = 0x0B,
		ArithmeticOverflow = 0x0C
	};

	struct Instruction {
		U32 Address;
		U32 binaryInstruction;
		String Mnemonic;
		std::function<void(const Instruction&)> Execute;

		U8 Opcode;
		U8 RegisterSource;
		U8 RegisterTarget;
		U8 RegisterDestination;
		U8 ShiftAmount;
		U8 Function;
		U16 Immediate;
		U32 Code;
		U32 PseudoAddress;
	};

	class R3000 : public BusDevice {
	public:
		R3000();
		~R3000();

		void clock();
	private:
		U32 fetch(U32 address);
		Instruction decode(U32 instruction);

		template<typename T>
		U32 load(U32 address) {
			if (ADDRESS_UNALIGNED(address,T)) {
				raiseException(ExceptionType::AddressErrorLoad);
			}

			U32 sr = getCP0Register((U8)COP0Register::SR);

			//Isc is 1
			if ((sr & (1 << 16)) != 0 && address < 0x1000) {
				U32 output = 0;

				for (size_t i = 0; i < sizeof(T); i++) {
					output <<= 8;
					output |= mICache[address + (sizeof(T) - 1 - i)];
				}

				return output;
			}

			return getBus(ESX_TEXT("Root"))->load<T>(address & SEGS_MASKS[address >> 29]);
		}

		template<typename T>
		void store(U32 address, U32 value) {
			if (ADDRESS_UNALIGNED(address, T)) {
				raiseException(ExceptionType::AddressErrorStore);
			}

			U32 sr = getCP0Register((U8)COP0Register::SR);

			//Isc is 1
			if ((sr & (1 << 16)) != 0 && address < 0x1000) {
				for (size_t i = 0; i < sizeof(T); i++) {
					mICache[address + i] = (value & 0xFF);
					value >>= 8;
				}

				return;
			}

			getBus(ESX_TEXT("Root"))->store<T>(address & SEGS_MASKS[address >> 29], value);
		}

		//Arithmetic
		void ADD(const Instruction& instruction);
		void ADDU(const Instruction& instruction);
		void SUB(const Instruction& instruction);
		void SUBU(const Instruction& instruction);
		void ADDI(const Instruction& instruction);
		void ADDIU(const Instruction& instruction);
		void MULT(const Instruction& instruction);
		void MULTU(const Instruction& instruction);
		void DIV(const Instruction& instruction);
		void DIVU(const Instruction& instruction);
		void MFLO(const Instruction& instruction);
		void MTLO(const Instruction& instruction);
		void MFHI(const Instruction& instruction);
		void MTHI(const Instruction& instruction);

		//Memory
		void LW(const Instruction& instruction);
		void LH(const Instruction& instruction);
		void LHU(const Instruction& instruction);
		void LB(const Instruction& instruction);
		void LBU(const Instruction& instruction);
		void LWL(const Instruction& instruction);
		void LWR(const Instruction& instruction);
		void SW(const Instruction& instruction);
		void SWL(const Instruction& instruction);
		void SWR(const Instruction& instruction);
		void SH(const Instruction& instruction);
		void SB(const Instruction& instruction);
		void LUI(const Instruction& instruction);

		//Comparison
		void SLT(const Instruction& instruction);
		void SLTU(const Instruction& instruction);
		void SLTI(const Instruction& instruction);
		void SLTIU(const Instruction& instruction);

		//Binary
		void AND(const Instruction& instruction);
		void ANDI(const Instruction& instruction);
		void OR(const Instruction& instruction);
		void ORI(const Instruction& instruction);
		void XOR(const Instruction& instruction);
		void XORI(const Instruction& instruction);
		void NOR(const Instruction& instruction);
		void SLL(const Instruction& instruction);
		void SRL(const Instruction& instruction);
		void SRA(const Instruction& instruction);
		void SLLV(const Instruction& instruction);
		void SRLV(const Instruction& instruction);
		void SRAV(const Instruction& instruction);

		//Control
		void BEQ(const Instruction& instruction);
		void BNE(const Instruction& instruction);
		void BLTZ(const Instruction& instruction);
		void BLTZAL(const Instruction& instruction);
		void BLEZ(const Instruction& instruction);
		void BGTZ(const Instruction& instruction);
		void BGEZ(const Instruction& instruction);
		void BGEZAL(const Instruction& instruction);
		void J(const Instruction& instruction);
		void JR(const Instruction& instruction);
		void JAL(const Instruction& instruction);
		void JALR(const Instruction& instruction);
		void BREAK(const Instruction& instruction);
		void SYSCALL(const Instruction& instruction);

		//COP0
		void MTC0(const Instruction& instruction);
		void MFC0(const Instruction& instruction);
		void RFE(const Instruction& instruction);

		void addPendingLoad(U8 index, U32 value);

		void setRegister(U8 index, U32 value);
		U32 getRegister(U8 index);

		void setCP0Register(U8 index, U32 value);
		U32 getCP0Register(U8 index);

		void raiseException(ExceptionType type, const Instruction* instruction = nullptr);
		void raiseBreakpoint();

	private:
		Array<U32, 32> mRegisters;
		Queue<Pair<U32, U32>> mPendingLoads;
		U32 mPC;
		U32 mHI, mLO;

		U32 mNextPC;

		Array<U32, 32> mCP0Registers;
		Vector<U8> mICache;
	};

}