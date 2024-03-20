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

	enum class GPRRegister : U8 {
		zero,
		at,
		v0, 
		v1,
		a0, 
		a1, 
		a2, 
		a3,
		t0,
		t1,
		t2, 
		t3, 
		t4,
		t5, 
		t6, 
		t7,
		s0, 
		s1, 
		s2, 
		s3, 
		s4, 
		s5, 
		s6, 
		s7,
		t8, 
		t9,
		k0, 
		k1,
		gp,
		sp,
		fp,
		ra
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

	struct Instruction;
	class R3000;
	typedef void(R3000::*ExecuteFunction)();

	struct RegisterIndex {
		I32 Value;

		RegisterIndex() : Value(-1) {}
		explicit RegisterIndex(U8 value) : Value(value) {}
		RegisterIndex(COP0Register r) : Value((U8)r) {}
		RegisterIndex(GPRRegister r) : Value((U8)r) {}

		operator U8() {
			return Value;
		}
	};

	struct Instruction {
		U32 Address = 0;
		U32 binaryInstruction = 0;
		ExecuteFunction Execute = nullptr;

		U8 Opcode() const {
			return ((binaryInstruction >> 26) & 0x3F);
		}

		RegisterIndex RegisterSource() const {
			return RegisterIndex(((binaryInstruction >> 21) & 0x1F));
		}

		RegisterIndex RegisterTarget() const {
			return RegisterIndex(((binaryInstruction >> 16) & 0x1F));
		}

		RegisterIndex RegisterDestination() const  {
			return RegisterIndex(((binaryInstruction >> 11) & 0x1F));
		}

		U8 ShiftAmount() const  {
			return ((binaryInstruction >> 6) & 0x1F);
		}

		U8 Function() const {
			return (binaryInstruction & 0x3F);
		}

		U16 Immediate() const {
			return (binaryInstruction & 0xFFFF);
		}

		U32 ImmediateSE() const {
			return (((binaryInstruction & 0xFFFF) & 0x8000) ? Immediate() | 0xFFFF0000 : Immediate());
		}

		U32 Code() const {
			return ((binaryInstruction >> 6) & 0xFFFFF);
		}

		U32 PseudoAddress() const {
			return (binaryInstruction & 0x3FFFFFF);
		}

		String Mnemonic() const;
	};

	class CPUStatusPanel;
	class DisassemblerPanel;

	
	class R3000 : public BusDevice {
	public:
		friend class CPUStatusPanel;
		friend class DisassemblerPanel;

		R3000();
		~R3000();

		void clock();
		U32 fetch(U32 address);
		void decode(Instruction& result, U32 instruction, U32 address, BIT suppressException = ESX_FALSE);

		template<typename T>
		U32 load(U32 address) {
			if (ADDRESS_UNALIGNED(address,T)) {
				raiseException(ExceptionType::AddressErrorLoad);
				return 0;
			}

			if (!mRootBus) mRootBus = getBus(ESX_TEXT("Root"));

			return mRootBus->load<T>(address & SEGS_MASKS[address >> 29]);
		}

		template<typename T>
		void store(U32 address, U32 value) {
			if (ADDRESS_UNALIGNED(address, T)) {
				raiseException(ExceptionType::AddressErrorStore);
				return;
			}

			if (!mRootBus) mRootBus = getBus(ESX_TEXT("Root"));

			mRootBus->store<T>(address & SEGS_MASKS[address >> 29], value);
		}

		//Arithmetic
		void ADD();
		void ADDU();
		void SUB();
		void SUBU();
		void ADDI();
		void ADDIU();
		void MULT();
		void MULTU();
		void DIV();
		void DIVU();
		void MFLO();
		void MTLO();
		void MFHI();
		void MTHI();

		//Memory
		void LW();
		void LH();
		void LHU();
		void LB();
		void LBU();
		void LWL();
		void LWR();
		void SW();
		void SWL();
		void SWR();
		void SH();
		void SB();
		void LUI();

		//Comparison
		void SLT();
		void SLTU();
		void SLTI();
		void SLTIU();

		//Binary
		void AND();
		void ANDI();
		void OR();
		void ORI();
		void XOR();
		void XORI();
		void NOR();
		void SLL();
		void SRL();
		void SRA();
		void SLLV();
		void SRLV();
		void SRAV();

		//Control
		void BEQ();
		void BNE();
		void BLTZ();
		void BLTZAL();
		void BLEZ();
		void BGTZ();
		void BGEZ();
		void BGEZAL();
		void J();
		void JR();
		void JAL();
		void JALR();
		void BREAK();
		void SYSCALL();

		//COPx
		void MTC0();
		void MFC0();
		void MTC2();
		void MFC2();
		void RFE();
		void LWC2();
		void SWC2();

	private:
		void addPendingLoad(RegisterIndex index, U32 value);

		void setRegister(RegisterIndex index, U32 value);
		U32 getRegister(RegisterIndex index);

		void setCP0Register(RegisterIndex index, U32 value);
		U32 getCP0Register(RegisterIndex index);

		void raiseException(ExceptionType type);
	private:
		SharedPtr<Bus> mRootBus;
		Array<U32, 32> mRegisters;
		Queue<Pair<RegisterIndex, U32>> mPendingLoads;
		U32 mPC = 0;
		U32 mNextPC = 0;
		U32 mCurrentPC = 0;
		U32 mHI = 0;
		U32 mLO = 0;
		Array<U32, 64> mCP0Registers;
		Instruction mCurrentInstruction;

		BIT mBranch = ESX_FALSE;
		BIT mBranchSlot = ESX_FALSE;
	};

}