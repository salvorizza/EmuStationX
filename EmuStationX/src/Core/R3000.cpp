#include "R3000.h"

#include <iostream>
#include <iomanip>
#include <fstream>

#include "Core/GPU.h"
#include "Core/Timer.h"
#include "Core/CDROM.h"
#include "Core/SIO.h"
#include "Core/InterruptControl.h"
#include "Core/DMA.h"

#include "optick.h"

namespace esx {

	R3000::R3000()
		:	BusDevice(ESX_TEXT("R3000")),
			mRegisters(),
			mOutRegisters(),
			mCP0Registers(),
			mPC(0xBFC00000)
	{
		mNextPC = mPC + 4;
		setCP0Register(COP0Register::PRId, 0x00000002);
		resetPendingLoad();
	}

	R3000::~R3000()
	{
	}

	void R3000::clock()
	{
		if (!mTimer) mTimer = getBus("Root")->getDevice<Timer>("Timer");
		if (!mCDROM) mCDROM = getBus("Root")->getDevice<CDROM>("CDROM");
		if (!mGPU) mGPU = getBus("Root")->getDevice<GPU>("GPU");
		if (!mSIO0) mSIO0 = getBus("Root")->getDevice<SIO>("SIO0");
		if (!mSIO1) mSIO1 = getBus("Root")->getDevice<SIO>("SIO1");
		if (!mInterruptControl) mInterruptControl = getBus("Root")->getDevice<InterruptControl>("InterruptControl");
		if (!mDMA) mDMA = getBus("Root")->getDevice<DMA>("DMA");

		if (!mStall) {
			U32 opcode = fetch(mPC);

			decode(mCurrentInstruction, opcode, mPC);

			//ESX_CORE_ASSERT(mCurrentInstruction.Execute, "No Operation");

			mCurrentPC = mPC;
			mPC = mNextPC;
			mNextPC += 4;

			mBranchSlot = mBranch;
			mTookBranchSlot = mTookBranch;
			mBranch = ESX_FALSE;
			mTookBranchSlot = ESX_FALSE;

			setRegister(mPendingLoad.first, mPendingLoad.second);
			resetPendingLoad();

			(this->*mCurrentInstruction.Execute)();

			/*if (mStall) {
				mNextPC = mPC;
				mPC = mCurrentPC;
				resetPendingLoad();
			} else {
				mRegisters = mOutRegisters;
			}*/

		}

		mGPU->clock(mCycles);
		mTimer->clock(mCycles);
		mCDROM->clock(mCycles);
		mSIO0->clock(mCycles);
		mDMA->clock(mCycles);
		if (!mDMA->isRunning()) {
			mStall = ESX_FALSE;
		}

		mInterruptControl->clock(mCycles);
		handleInterrupts();

		mCycles++;
	}


	U32 R3000::fetch(U32 address)
	{
		if (isCacheActive(address)) {
			if (ADDRESS_UNALIGNED(address, U32)) {
				raiseException(ExceptionType::AddressErrorLoad);
				return 0;
			}

			U32 index = (address >> 2) & 0x3;
			U32 cacheLineNumber = (address >> 4) & 0xFF;
			U32 tag = address >> 12;

			auto& cacheLine = mICache.CacheLines[cacheLineNumber];
			if (cacheLine.Tag == tag) {
				auto& instruction = cacheLine.Instructions[index];
				if (instruction.Valid) {
					return instruction.Word;
				}
				else {
					return cacheMiss(address, cacheLineNumber, tag, index);
				}
			}
			else {
				return cacheMiss(address, cacheLineNumber, tag, index);
			}
		} else {
			if (!mRootBus) mRootBus = getBus(ESX_TEXT("Root"));
			return mRootBus->load<U32>(toPhysicalAddress(address));
		}
	}

	static const Array<ExecuteFunction, 64> primaryOpCodeDecode = {
		&R3000::NA, &R3000::NA, &R3000::J, &R3000::JAL, &R3000::BEQ, &R3000::BNE, &R3000::BLEZ, &R3000::BGTZ,
		&R3000::ADDI, &R3000::ADDIU, &R3000::SLTI, &R3000::SLTIU, &R3000::ANDI, &R3000::ORI, &R3000::XORI, &R3000::LUI,
		&R3000::COP0, &R3000::COP1, &R3000::COP2, &R3000::COP3, &R3000::NA, &R3000::NA, &R3000::NA, &R3000::NA,
		&R3000::NA, &R3000::NA, &R3000::NA, &R3000::NA, &R3000::NA, &R3000::NA, &R3000::NA, &R3000::NA,
		&R3000::LB, &R3000::LH, &R3000::LWL, &R3000::LW, &R3000::LBU, &R3000::LHU, &R3000::LWR, &R3000::NA,
		&R3000::SB, &R3000::SH, &R3000::SWL, &R3000::SW, &R3000::NA, &R3000::NA, &R3000::SWR, &R3000::NA,
		&R3000::LWC0, &R3000::LWC1, &R3000::LWC2, &R3000::LWC3, &R3000::NA, &R3000::NA, &R3000::NA, &R3000::NA,
		&R3000::SWC0, &R3000::SWC1, &R3000::SWC2, &R3000::SWC3, &R3000::NA, &R3000::NA, &R3000::NA, &R3000::NA
	};

	static const Array<ExecuteFunction, 64> secondaryOpCodeDecode = {
		&R3000::SLL, &R3000::NA, &R3000::SRL, &R3000::SRA, &R3000::SLLV, &R3000::NA, &R3000::SRLV, &R3000::SRAV,
		&R3000::JR, &R3000::JALR, &R3000::NA, &R3000::NA, &R3000::SYSCALL, &R3000::BREAK, &R3000::NA, &R3000::NA,
		&R3000::MFHI, &R3000::MTHI, &R3000::MFLO, &R3000::MTLO, &R3000::NA, &R3000::NA, &R3000::NA, &R3000::NA,
		&R3000::MULT, &R3000::MULTU, &R3000::DIV, &R3000::DIVU, &R3000::NA, &R3000::NA, &R3000::NA, &R3000::NA,
		&R3000::ADD, &R3000::ADDU, &R3000::SUB, &R3000::SUBU, &R3000::AND, &R3000::OR, &R3000::XOR, &R3000::NOR,
		&R3000::NA, &R3000::NA, &R3000::SLT, &R3000::SLTU, &R3000::NA, &R3000::NA, &R3000::NA, &R3000::NA,
		&R3000::NA, &R3000::NA, &R3000::NA, &R3000::NA, &R3000::NA, &R3000::NA, &R3000::NA, &R3000::NA,
		&R3000::NA, &R3000::NA, &R3000::NA, &R3000::NA, &R3000::NA, &R3000::NA, &R3000::NA, &R3000::NA
	};

	static const Array<ExecuteFunction, 4> branchOpCodeDecode = {
		&R3000::BLTZ,
		&R3000::BGEZ,
		&R3000::BLTZAL,
		&R3000::BGEZAL
	};

	void R3000::decode(Instruction& result, U32 instruction, U32 address, BIT suppressException)
	{
		result.Address = address;
		result.binaryInstruction = instruction;
		result.Execute = nullptr;

		U32 primaryOpCode = result.Opcode();
		U32 secondaryOpCode = result.Function();

		if (primaryOpCode == 0x00) {
			result.Execute = secondaryOpCodeDecode[secondaryOpCode];
		} else if (primaryOpCode == 0x01) {
			result.Execute = branchOpCodeDecode[result.RegisterTarget().Value];
		} else {
			result.Execute = primaryOpCodeDecode[primaryOpCode];
		}
	}

	void R3000::ADD()
	{
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 b = getRegister(mCurrentInstruction.RegisterTarget());

		I32 r = a + b;

		if (OVERFLOW_ADD32(a, b, r)) {
			raiseException(ExceptionType::ArithmeticOverflow);
			return;
		}

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R3000::ADDU()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = getRegister(mCurrentInstruction.RegisterTarget());

		U32 r = a + b;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R3000::SUB()
	{
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 b = getRegister(mCurrentInstruction.RegisterTarget());

		I32 r = a - b;

		if (OVERFLOW_SUB32(a, b, r)) {
			raiseException(ExceptionType::ArithmeticOverflow);
			return;
		}

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R3000::SUBU()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = getRegister(mCurrentInstruction.RegisterTarget());

		U32 r = a - b;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R3000::ADDI()
	{
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 b = mCurrentInstruction.ImmediateSE();

		I32 r = a + b;

		if (OVERFLOW_ADD32(a, b, r)) {
			raiseException(ExceptionType::ArithmeticOverflow);
			return;
		}

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void R3000::ADDIU()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();

		U32 r = a + b;

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void R3000::MULT()
	{
		I64 a = getRegister(mCurrentInstruction.RegisterSource());
		I64 b = getRegister(mCurrentInstruction.RegisterTarget());

		I64 r = a * b;

		mHI = (r >> 32) & 0xFFFFFFFF;
		mLO = r & 0xFFFFFFFF;
	}

	void R3000::MULTU()
	{
		U64 a = getRegister(mCurrentInstruction.RegisterSource());
		U64 b = getRegister(mCurrentInstruction.RegisterTarget());

		U64 r = a * b;

		mHI = (r >> 32) & 0xFFFFFFFF;
		mLO = r & 0xFFFFFFFF;
	}

	void R3000::DIV()
	{
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 b = getRegister(mCurrentInstruction.RegisterTarget());

		if (b != 0) {
			if (a == 0x80000000 && b == -1) {
				mHI = 0;
				mLO = 0x80000000;
			} else {
				mHI = a % b;
				mLO = a / b;
			}
		} else {
			mHI = a;

			if (a >= 0) {
				mLO = 0xFFFFFFFF;
			} else {
				mLO = 1;
			}
		}
	}

	void R3000::DIVU()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = getRegister(mCurrentInstruction.RegisterTarget());

		if (b != 0) {
			mHI = a % b;
			mLO = a / b;
		} else {
			mHI = a;
			mLO = 0xFFFFFFFF;
		}
	}

	void R3000::MFLO()
	{
		addPendingLoad(mCurrentInstruction.RegisterDestination(), mLO);
	}

	void R3000::MTLO()
	{
		mLO = getRegister(mCurrentInstruction.RegisterSource());
	}

	void R3000::MFHI()
	{
		addPendingLoad(mCurrentInstruction.RegisterDestination(), mHI);
	}

	void R3000::MTHI()
	{
		mHI = getRegister(mCurrentInstruction.RegisterSource());
	}

	void R3000::LW()
	{
		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated load from {:08x} not handled", m);
			return;
		}

		U32 r = load<U32>(m);

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
	}

	void R3000::LH()
	{
		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated load from {:08x} not handled", m);
			return;
		}

		U32 r = load<U16>(m);
		r = SIGNEXT16(r);

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
	}

	void R3000::LHU()
	{
		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated load from {:08x} not handled", m);
			return;
		}

		U32 r = load<U16>(m);

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
	}

	void R3000::LB()
	{
		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated load from {:08x} not handled", m);
			return;
		}

		U32 r = load<U8>(m);
		r = SIGNEXT8(r);

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
	}

	void R3000::LBU()
	{
		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated load from {:08x} not handled", m);
			return;
		}

		U32 r = load<U8>(m);

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
	}

	void R3000::LWL()
	{
		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();
		U32 c = getRegister(mCurrentInstruction.RegisterTarget());

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated load from {:08x} not handled", m);
			return;
		}

		U32 am = m & ~(0x3);
		U32 aw = load<U32>(am);

		U32 u = m & (0x3);
		U32 r = (c & (0x00FFFFFF >> (u * 8))) | (aw << (24 - (u * 8)));

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
	}

	void R3000::SWL()
	{
		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();
		U32 c = getRegister(mCurrentInstruction.RegisterTarget());

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated load from {:08x} not handled", m);
			return;
		}

		U32 am = m & ~(0x3);
		U32 aw = load<U32>(am);

		U32 u = (m & 0x3) * 8;
		U32 mr = (aw & (0xFFFFFF00 << u)) | (c >> (24 - u));

		store<U32>(am, mr);
	}


	void R3000::LWR()
	{
		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();
		U32 c = getRegister(mCurrentInstruction.RegisterTarget());

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated load from {:08x} not handled", m);
			return;
		}

		U32 am = m & ~(0x3);
		U32 aw = load<U32>(am);

		U32 u = m & 0x3;
		U32 r = (c & (0xFFFFFF00 << ((0x3 - u) * 8))) | (aw >> (u * 8));

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
	}

	void R3000::SWR()
	{
		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();
		U32 c = getRegister(mCurrentInstruction.RegisterTarget());

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated store to {:08x} not handled", m);
			return;
		}

		U32 am = m & ~(0x3);
		U32 aw = load<U32>(am);

		U32 u = m & 0x3;
		U32 mr = (aw & (0x00FFFFFF >> ((0x3 - u) * 8))) | (c << (u * 8));

		store<U32>(am, mr);
	}

	void R3000::SB()
	{
		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();
		U32 v = getRegister(mCurrentInstruction.RegisterTarget());

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated store to {:08x} not handled", m);
			return;
		}
		
		store<U8>(m, v);
	}

	void R3000::SH()
	{
		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();
		U32 v = getRegister(mCurrentInstruction.RegisterTarget());

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated store to {:08x} not handled", m);
			return;
		}

		store<U16>(m, v);
	}

	void R3000::SW()
	{
		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated store to {:08x} not handled", m);
			return;
		}

		U32 v = getRegister(mCurrentInstruction.RegisterTarget());

		store<U32>(m, v);
	}

	void R3000::LUI()
	{
		U32 r = mCurrentInstruction.Immediate() << 16;
		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void R3000::SLT()
	{
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 b = getRegister(mCurrentInstruction.RegisterTarget());

		U32 r = a < b;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R3000::SLTU()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = getRegister(mCurrentInstruction.RegisterTarget());

		U32 r = a < b;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R3000::SLTI()
	{
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 b = mCurrentInstruction.ImmediateSE();

		U32 r = a < b;

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void R3000::SLTIU()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();

		U32 r = a < b;

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void R3000::AND()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = getRegister(mCurrentInstruction.RegisterTarget());

		U32 r = a & b;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R3000::ANDI()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.Immediate();

		U32 r = a & b;

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void R3000::OR()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = getRegister(mCurrentInstruction.RegisterTarget());

		U32 r = a | b;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R3000::ORI()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.Immediate();

		U32 r = a | b;

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void R3000::XOR()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = getRegister(mCurrentInstruction.RegisterTarget());

		U32 r = a ^ b;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R3000::XORI()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.Immediate();

		U32 r = a ^ b;

		setRegister(mCurrentInstruction.RegisterTarget(), r);
	}

	void R3000::NOR()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = getRegister(mCurrentInstruction.RegisterTarget());

		U32 r = ~(a | b);

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R3000::SLL()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = mCurrentInstruction.ShiftAmount();

		U32 r = a << s;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R3000::SRL()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = mCurrentInstruction.ShiftAmount();

		U32 r = a >> s;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R3000::SRA()
	{
		I32 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = mCurrentInstruction.ShiftAmount();

		U32 r = a >> s;

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R3000::SLLV()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = getRegister(mCurrentInstruction.RegisterSource());

		U32 r = a << (s & 0x1F);

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R3000::SRLV()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = getRegister(mCurrentInstruction.RegisterSource());

		U32 r = a >> (s & 0x1F);

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R3000::SRAV()
	{
		I32 a = getRegister(mCurrentInstruction.RegisterTarget());
		U32 s = getRegister(mCurrentInstruction.RegisterSource());

		U32 r = a >> (s & 0x1F);

		setRegister(mCurrentInstruction.RegisterDestination(), r);
	}

	void R3000::BEQ()
	{
		mBranch = ESX_TRUE;
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = getRegister(mCurrentInstruction.RegisterTarget());
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		if (a == b) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void R3000::BNE()
	{
		mBranch = ESX_TRUE;
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = getRegister(mCurrentInstruction.RegisterTarget());
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		if (a != b) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void R3000::BLTZ()
	{
		mBranch = ESX_TRUE;
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		if (a < 0) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void R3000::BLTZAL()
	{
		mBranch = ESX_TRUE;
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		if (a < 0) {
			setRegister(GPRRegister::ra, mNextPC);
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void R3000::BLEZ()
	{
		mBranch = ESX_TRUE;
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		if (a <= 0) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void R3000::BGTZ()
	{
		mBranch = ESX_TRUE;
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		if (a > 0) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void R3000::BGEZ()
	{
		mBranch = ESX_TRUE;
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		if (a >= 0) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void R3000::BGEZAL()
	{
		mBranch = ESX_TRUE;
		I32 a = getRegister(mCurrentInstruction.RegisterSource());
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		if (a >= 0) {
			setRegister(GPRRegister::ra, mNextPC);
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void R3000::J()
	{
		U32 a = (mNextPC & 0xF0000000) | (mCurrentInstruction.PseudoAddress() << 2);
		mNextPC = a;
		mBranch = ESX_TRUE;
		mTookBranch = ESX_TRUE;
	}

	void R3000::JR()
	{
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		mNextPC = a;
		mBranch = ESX_TRUE;
		mTookBranch = ESX_TRUE;
	}

	void R3000::JAL()
	{
		setRegister(GPRRegister::ra, mNextPC);
		J();
	}

	void R3000::JALR()
	{
		setRegister(GPRRegister::ra, mNextPC);
		JR();
	}

	void R3000::BREAK()
	{
		ESX_CORE_LOG_ERROR("0x{:08X} Break", mCurrentInstruction.Address);
		raiseException(ExceptionType::Breakpoint);
	}

	void R3000::SYSCALL()
	{
		raiseException(ExceptionType::Syscall);
	}

	void R3000::COP0()
	{
		if (CO(mCurrentInstruction.binaryInstruction) == 0) {
			switch (mCurrentInstruction.RegisterSource().Value) {
				case 0x00: {
					MFC0();
					break;
				}
				case 0x04: {
					MTC0();
					break;
				}
				case 0x08: {
					if (mCurrentInstruction.RegisterTarget().Value == 1) {
						BC0T();
					} else if (mCurrentInstruction.RegisterTarget().Value == 0) {
						BC0F();
					}
					break;
				}
				default: {
					raiseException(ExceptionType::CoprocessorUnusable);
					break;
				}
			}
		}
		else {
			switch (COP_FUNC(mCurrentInstruction.binaryInstruction)) {
				case 0x10: {
					RFE();
					break;
				}

				default: {
					raiseException(ExceptionType::ReservedInstruction);
					break;
				}
			}
		}
	}

	void R3000::COP1()
	{
		raiseException(ExceptionType::CoprocessorUnusable);
	}

	void R3000::COP2()
	{
		if (CO(mCurrentInstruction.binaryInstruction) == 0) {
			switch (mCurrentInstruction.RegisterSource().Value) {
				case 0x00: {
					MFC2();
					break;
				}
				case 0x02: {
					CFC2();
					break;
				}
				case 0x04: {
					MTC2();
					break;
				}
				case 0x06: {
					CTC2();
					break;
				}
				case 0x08: {
					if (mCurrentInstruction.RegisterTarget().Value == 1) {
						BC2T();
					}
					else if (mCurrentInstruction.RegisterTarget().Value == 0) {
						BC2F();
					}
					break;
				}
				default: {
					raiseException(ExceptionType::CoprocessorUnusable);
					break;
				}
			}
		}
		else {
			ESX_CORE_LOG_ERROR("GTE command {:08X}h Not implemented yet", mCurrentInstruction.Immediate25());
		}
	}

	void R3000::COP3()
	{
		raiseException(ExceptionType::CoprocessorUnusable);
	}

	void R3000::MTC0()
	{
		U32 sr = getCP0Register(COP0Register::SR);
		U32 r = getRegister(mCurrentInstruction.RegisterTarget());

		if (mCurrentInstruction.RegisterDestination() <= 2 ||
			mCurrentInstruction.RegisterDestination() == 4 ||
			mCurrentInstruction.RegisterDestination() == 10 ||
			(mCurrentInstruction.RegisterDestination() >= 32 && mCurrentInstruction.RegisterDestination() <= 63)) {
			raiseException(ExceptionType::ReservedInstruction);
			return;
		}

		if (mCurrentInstruction.RegisterDestination() < 16 && (sr & 0x10000002) == 0x1) {
			raiseException(ExceptionType::CoprocessorUnusable);
			return;
		}

		switch ((COP0Register)(U8)mCurrentInstruction.RegisterDestination()) {
			case COP0Register::Cause: {
				U32 t = getCP0Register(mCurrentInstruction.RegisterDestination());

				t &= ~0x300;
				r &= 0x300;

				r |= t;
				break;
			}

			case COP0Register::EPC:
			case COP0Register::BadVAddr:
			case COP0Register::PRId:
			case COP0Register::JumpDest:
				return;

		}

		setCP0Register(mCurrentInstruction.RegisterDestination(), r);
	}

	void R3000::MFC0()
	{
		U32 sr = getCP0Register(COP0Register::SR);
		U32 r = getCP0Register(mCurrentInstruction.RegisterDestination());

		if (mCurrentInstruction.RegisterDestination() < 16 && (sr & 0x10000002) == 0x1) {
			raiseException(ExceptionType::CoprocessorUnusable);
			return;
		}

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
	}

	void R3000::CFC2()
	{
		ESX_CORE_LOG_ERROR("GTE Copy from cop2.{} to gpr.{} not implemented yet", mCurrentInstruction.RegisterTarget().Value, mCurrentInstruction.RegisterDestination().Value);
	}

	void R3000::MTC2()
	{
		ESX_CORE_LOG_ERROR("GTE Move from gpr.{} to cop2.{} not implemented yet", mCurrentInstruction.RegisterTarget().Value, mCurrentInstruction.RegisterDestination().Value);
	}

	void R3000::MFC2()
	{
		ESX_CORE_LOG_ERROR("GTE Move from cop2.{} to gpr.{} not implemented yet", mCurrentInstruction.RegisterTarget().Value, mCurrentInstruction.RegisterDestination().Value);
	}

	void R3000::CTC2()
	{
		ESX_CORE_LOG_ERROR("GTE Copy from gpr.{} to cop2.{} not implemented yet", mCurrentInstruction.RegisterTarget().Value, mCurrentInstruction.RegisterDestination().Value);
	}

	void R3000::BC0F()
	{
		mBranch = ESX_TRUE;
		U8 a = getCP0Register(COP0Register::SR) & (1 << 6);
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		if (a == 0) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void R3000::BC2F()
	{
		ESX_CORE_LOG_ERROR("GTE Not implemented yet");
	}

	void R3000::BC0T()
	{
		mBranch = ESX_TRUE;
		U8 a = getCP0Register(COP0Register::SR) & (1 << 6);
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		if (a == 1) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void R3000::BC2T()
	{
		ESX_CORE_LOG_ERROR("GTE Not implemented yet");
	}

	void R3000::RFE()
	{
		U32 sr = getCP0Register(COP0Register::SR);

		if ((sr & 0x10000002) == 0x1) {
			raiseException(ExceptionType::CoprocessorUnusable);
			return;
		}

		U32 mode = sr & 0x3F;
		sr &= ~0x3F;
		sr |= (mode >> 2) & 0x3F;

		setCP0Register(COP0Register::SR, sr);
	}

	void R3000::LWC0()
	{
		raiseException(ExceptionType::CoprocessorUnusable);
	}

	void R3000::LWC1()
	{
		raiseException(ExceptionType::CoprocessorUnusable);
	}

	void R3000::LWC2()
	{
		ESX_CORE_LOG_ERROR("GTE Not implemented yet");
	}

	void R3000::LWC3()
	{
		raiseException(ExceptionType::CoprocessorUnusable);
	}

	void R3000::SWC0()
	{
		raiseException(ExceptionType::CoprocessorUnusable);
	}

	void R3000::SWC1()
	{
		raiseException(ExceptionType::CoprocessorUnusable);
	}

	void R3000::SWC2()
	{
		ESX_CORE_LOG_ERROR("GTE Not implemented yet");
	}

	void R3000::SWC3()
	{
		raiseException(ExceptionType::CoprocessorUnusable);
	}

	void R3000::NA()
	{
		raiseException(ExceptionType::ReservedInstruction);
	}

	void R3000::addPendingLoad(RegisterIndex index, U32 value)
	{
		mPendingLoad.first = index;
		mPendingLoad.second = value;
	}

	void R3000::resetPendingLoad()
	{
		mPendingLoad.first = RegisterIndex(0);
		mPendingLoad.second = 0;
	}

	void R3000::setRegister(RegisterIndex index, U32 value)
	{
		mOutRegisters[index.Value] = value;
		mOutRegisters[(U8)GPRRegister::zero] = 0;
	}

	U32 R3000::getRegister(RegisterIndex index)
	{
		return mOutRegisters[index.Value];
	}

	void R3000::setCP0Register(RegisterIndex index, U32 value)
	{
		mCP0Registers[index.Value] = value;
	}

	U32 R3000::getCP0Register(RegisterIndex index)
	{
		return mCP0Registers[index.Value];
	}

	U32 R3000::cacheMiss(U32 address, U32 cacheLineNumber, U32 tag, U32 startIndex)
	{
		if (!mRootBus) mRootBus = getBus(ESX_TEXT("Root"));

		auto& cacheLine = mICache.CacheLines[cacheLineNumber];
		cacheLine.Tag = tag;

		for (U32 index = 0; index < 4; index++) {
			auto& instruction = cacheLine.Instructions[index];

			if (index >= startIndex) {
				U32 word = mRootBus->load<U32>(toPhysicalAddress(address + (index - startIndex) * 4));
				instruction.Word = word;
				instruction.Valid = ESX_TRUE;
			} else {
				instruction.Valid = ESX_FALSE;
			}
		}

		return cacheLine.Instructions[startIndex].Word;
	}

	void R3000::handleInterrupts()
	{
		U32 cause = getCP0Register(COP0Register::Cause);
		U32 sr = getCP0Register(COP0Register::SR);

		if (mInterruptControl->interruptPending()) {
			cause |= (1 << 10);
		} else {
			cause &= ~(1 << 10);
		}

		setCP0Register(COP0Register::Cause, cause);

		BIT IEC = sr & 0x1;
		U8 IM = (sr >> 8) & 0xFF;
		U8 IP = (cause >> 8) & 0xFF;

		if (IEC && ((IM & IP) > 0)) {
			raiseException(ExceptionType::Interrupt);
		}
	}

	void R3000::raiseException(ExceptionType type)
	{
		U32 sr = getCP0Register(COP0Register::SR);
		U32 epc = getCP0Register(COP0Register::EPC);
		U32 cause = getCP0Register(COP0Register::Cause);

		U32 handler = 0x80000080;
		if ((sr & (1 << 22)) != 0) {
			handler = 0xBFC00180;
		}

		U32 mode = sr & 0x3F;
		sr &= ~0x3F;
		sr |= (mode << 2) & 0x3F;

		cause = ((U32)type) << 2;

		if (type == ExceptionType::Interrupt) {
			epc = mPC;
			mBranchSlot = mBranch;
			mTookBranchSlot = mTookBranch;
		} else {
			epc = mCurrentPC;
		}

		if (mBranchSlot) {
			epc -= 4;
			cause |= 1 << 31;
			setCP0Register(COP0Register::JumpDest, mPC);

			if (mTookBranchSlot) {
				cause |= 1 << 30;
			}
		}

		setCP0Register(COP0Register::Cause, cause);
		setCP0Register(COP0Register::EPC, epc);
		setCP0Register(COP0Register::SR, sr);

		mPC = handler;
		mNextPC = mPC + 4;
	}

	String Instruction::Mnemonic() const
	{
		constexpr static std::array<StringView,32> registersMnemonics = {
			ESX_TEXT("$zero"),
			ESX_TEXT("$at"),
			ESX_TEXT("$v0"),ESX_TEXT("$v1"),
			ESX_TEXT("$a0"),ESX_TEXT("$a1"),ESX_TEXT("$a2"),ESX_TEXT("$a3"),
			ESX_TEXT("$t0"),ESX_TEXT("$t1"),ESX_TEXT("$t2"),ESX_TEXT("$t3"),ESX_TEXT("$t4"),ESX_TEXT("$t5"),ESX_TEXT("$t6"),ESX_TEXT("$t7"),
			ESX_TEXT("$s0"),ESX_TEXT("$s1"),ESX_TEXT("$s2"),ESX_TEXT("$s3"),ESX_TEXT("$s4"),ESX_TEXT("$s5"),ESX_TEXT("$s6"),ESX_TEXT("$s7"),
			ESX_TEXT("$t8"),ESX_TEXT("$t9"),
			ESX_TEXT("$k0"),ESX_TEXT("$k1"),
			ESX_TEXT("$gp"),
			ESX_TEXT("$sp"),
			ESX_TEXT("$fp"),
			ESX_TEXT("$ra")
		};

		switch (Opcode()) {
			//R Type
			case 0x00: {
				switch (Function()) {
					case 0x00: {
						return FormatString(ESX_TEXT("sll {},{},0x{:02x}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterTarget()], ShiftAmount());
					}
					case 0x02: {
						return FormatString(ESX_TEXT("srl {},{},0x{:02x}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterTarget()], ShiftAmount());
					}
					case 0x03: {
						return FormatString(ESX_TEXT("sra {},{},0x{:02x}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterTarget()], ShiftAmount());
					}
					case 0x04: {
						return FormatString(ESX_TEXT("sllv {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()]);
					}
					case 0x06: {
						return FormatString(ESX_TEXT("srlv {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()]);
					}
					case 0x07: {
						return FormatString(ESX_TEXT("srav {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()]);
					}
					case 0x08: {
						return FormatString(ESX_TEXT("jr {}"), registersMnemonics[(U8)RegisterSource()]);
					}
					case 0x09: {
						return FormatString(ESX_TEXT("jalr {},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()]);
					}
					case 0x0C: {
						return FormatString(ESX_TEXT("syscall"));
					}
					case 0x0D: {
						return FormatString(ESX_TEXT("break"));
					}
					case 0x10: {
						return FormatString(ESX_TEXT("mfhi {}"), registersMnemonics[(U8)RegisterDestination()]);
					}
					case 0x11: {
						return FormatString(ESX_TEXT("mthi {}"), registersMnemonics[(U8)RegisterSource()]);
					}
					case 0x12: {
						return FormatString(ESX_TEXT("mflo {}"), registersMnemonics[(U8)RegisterDestination()]);
					}
					case 0x13: {
						return FormatString(ESX_TEXT("mtlo {}"), registersMnemonics[(U8)RegisterSource()]);
					}
					case 0x18: {
						return FormatString(ESX_TEXT("mult {},{}"), registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x19: {
						return FormatString(ESX_TEXT("multu {},{}"), registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x1A: {
						return FormatString(ESX_TEXT("div {},{}"), registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x1B: {
						return FormatString(ESX_TEXT("divu {},{}"), registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x20: {
						if (RegisterTarget().Value == 0) {
							return FormatString(ESX_TEXT("move {},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()]);
						}
						else {
							return FormatString(ESX_TEXT("add {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
						}
					}
					case 0x21: {
						if (RegisterTarget().Value == 0) {
							return FormatString(ESX_TEXT("move {},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()]);
						}
						else {
							return FormatString(ESX_TEXT("addu {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
						}
					}
					case 0x22: {
						return FormatString(ESX_TEXT("sub {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x23: {
						return FormatString(ESX_TEXT("subu {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x24: {
						return FormatString(ESX_TEXT("and {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x25: {
						return FormatString(ESX_TEXT("or {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x26: {
						return FormatString(ESX_TEXT("xor {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x27: {
						return FormatString(ESX_TEXT("nor {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x2A: {
						return FormatString(ESX_TEXT("slt {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
					case 0x2B: {
						return FormatString(ESX_TEXT("sltu {},{},{}"), registersMnemonics[(U8)RegisterDestination()], registersMnemonics[(U8)RegisterSource()], registersMnemonics[(U8)RegisterTarget()]);
					}
				}

				break;
			}

			//J Type
			case 0x02: {
				return FormatString(ESX_TEXT("j 0x{:08x}"), ((Address + 4) & 0xF0000000) | (PseudoAddress() << 2));
			}
			case 0x03: {
				return FormatString(ESX_TEXT("jal 0x{:08x}"), ((Address + 4) & 0xF0000000) | (PseudoAddress() << 2));
			}

			default: {
				switch (Opcode()) {
					case 0x01: {
						switch (RegisterTarget().Value) {
							case 0x00: {
								return FormatString(ESX_TEXT("bltz {},0x{:08x}"), registersMnemonics[(U8)RegisterSource()], (Address + 4) + (ImmediateSE() << 2));
							}
							case 0x01: {
								return FormatString(ESX_TEXT("bgez {},0x{:08x}"), registersMnemonics[(U8)RegisterSource()], (Address + 4) + (ImmediateSE() << 2));
							}
							case 0x10: {
								return FormatString(ESX_TEXT("bltzal {},0x{:08x}"), registersMnemonics[(U8)RegisterSource()], (Address + 4) + (ImmediateSE() << 2));
							}
							case 0x11: {
								return FormatString(ESX_TEXT("bgezal {},0x{:08x}"), registersMnemonics[(U8)RegisterSource()], (Address + 4) + (ImmediateSE() << 2));
							}
						}

						break;
					}
					case 0x04: {
						return FormatString(ESX_TEXT("beq {},{},0x{:08x}"), registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()], (Address + 4) + (ImmediateSE() << 2));
					}
					case 0x05: {
						return FormatString(ESX_TEXT("bne {},{},0x{:08x}"), registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()], (Address + 4) + (ImmediateSE() << 2));
					}
					case 0x06: {
						return FormatString(ESX_TEXT("blez {},0x{:08x}"), registersMnemonics[(U8)RegisterSource()], (Address + 4) + (ImmediateSE() << 2));
					}
					case 0x07: {
						return FormatString(ESX_TEXT("bgtz {},0x{:08x}"), registersMnemonics[(U8)RegisterSource()], (Address + 4) + (ImmediateSE() << 2));
					}
					case 0x08: {
						return FormatString(ESX_TEXT("addi {},{},0x{:04x}"), registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()], (I16)Immediate());
					}
					case 0x09: {
						return FormatString(ESX_TEXT("addiu {},{},0x{:04x}"), registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()], (I16)Immediate());
					}
					case 0x0A: {
						return FormatString(ESX_TEXT("slti {},{},0x{:04x}"), registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()], (I16)Immediate());
					}
					case 0x0B: {
						return FormatString(ESX_TEXT("sltiu {},{},0x{:04x}"), registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()], (I16)Immediate());
					}
					case 0x0C: {
						return FormatString(ESX_TEXT("andi {},{},0x{:04x}"), registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()], Immediate());
					}
					case 0x0D: {
						return FormatString(ESX_TEXT("ori {},{},0x{:04x}"), registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()], Immediate());
					}
					case 0x0E: {
						return FormatString(ESX_TEXT("xori {},{},0x{:04x}"), registersMnemonics[(U8)RegisterTarget()], registersMnemonics[(U8)RegisterSource()], Immediate());
					}
					case 0x0F: {
						return FormatString(ESX_TEXT("lui {},0x{:04x}"), registersMnemonics[(U8)RegisterTarget()], Immediate());
					}
					case 0x10:
					case 0x11:
					case 0x12:
					case 0x13: {
						U8 cpn = CO_N(binaryInstruction);
						if (CO(binaryInstruction) == 0) {
							switch (RegisterSource().Value) {
								case 0x00: {
									return FormatString(ESX_TEXT("mfc{} {},${}"), cpn, registersMnemonics[(U8)RegisterTarget()], (U8)RegisterDestination());
								}
								case 0x02: {
									return FormatString(ESX_TEXT("cfc{} {},${}"), cpn, registersMnemonics[(U8)RegisterTarget()], (U8)RegisterDestination());
								}
								case 0x04: {
									return FormatString(ESX_TEXT("mtc{} {},${}"), cpn, registersMnemonics[(U8)RegisterTarget()], (U8)RegisterDestination());
								}
								case 0x06: {
									return FormatString(ESX_TEXT("ctc{} {},${}"), cpn, registersMnemonics[(U8)RegisterTarget()], (U8)RegisterDestination());
								}
								case 0x08: {
									switch (RegisterTarget()) {
										case 0: return FormatString(ESX_TEXT("bc{}f 0x{:04x}"), cpn, Immediate());
										case 1: return FormatString(ESX_TEXT("bc{}t 0x{:04x}"), cpn, Immediate());
									}
								}
							}
						}
						else {
							switch (cpn) {
								case 0: {
									switch (COP_FUNC(binaryInstruction)) {
										case 0x10: {
											return FormatString(ESX_TEXT("rfe"));
										}
									}
									break;
								}

								case 2: {
									return FormatString(ESX_TEXT("cop2 0x{:08x}"), Immediate25());
								}
							}
						}
						break;
					}
					case 0x20: {
						return FormatString(ESX_TEXT("lb {},0x{:04x}({})"), registersMnemonics[(U8)RegisterTarget()], Immediate(), registersMnemonics[(U8)RegisterSource()]);
					}
					case 0x21: {
						return FormatString(ESX_TEXT("lh {},0x{:04x}({})"), registersMnemonics[(U8)RegisterTarget()], Immediate(), registersMnemonics[(U8)RegisterSource()]);
					}
					case 0x22: {
						return FormatString(ESX_TEXT("lwl {},0x{:04x}({})"), registersMnemonics[(U8)RegisterTarget()], Immediate(), registersMnemonics[(U8)RegisterSource()]);
					}
					case 0x23: {
						return FormatString(ESX_TEXT("lw {},0x{:04x}({})"), registersMnemonics[(U8)RegisterTarget()], Immediate(), registersMnemonics[(U8)RegisterSource()]);
					}
					case 0x24: {
						return FormatString(ESX_TEXT("lbu {},0x{:04x}({})"), registersMnemonics[(U8)RegisterTarget()], Immediate(), registersMnemonics[(U8)RegisterSource()]);
					}
					case 0x25: {
						return FormatString(ESX_TEXT("lhu {},0x{:04x}({})"), registersMnemonics[(U8)RegisterTarget()], Immediate(), registersMnemonics[(U8)RegisterSource()]);
					}
					case 0x26: {
						return FormatString(ESX_TEXT("lwr {},0x{:04x}({})"), registersMnemonics[(U8)RegisterTarget()], Immediate(), registersMnemonics[(U8)RegisterSource()]);
					}
					case 0x28: {
						return FormatString(ESX_TEXT("sb {},0x{:04x}({})"), registersMnemonics[(U8)RegisterTarget()], Immediate(), registersMnemonics[(U8)RegisterSource()]);
					}
					case 0x29: {
						return FormatString(ESX_TEXT("sh {},0x{:04x}({})"), registersMnemonics[(U8)RegisterTarget()], Immediate(), registersMnemonics[(U8)RegisterSource()]);
					}
					case 0x2A: {
						return FormatString(ESX_TEXT("swl {},0x{:04x}({})"), registersMnemonics[(U8)RegisterTarget()], Immediate(), registersMnemonics[(U8)RegisterSource()]);
					}
					case 0x2B: {
						return FormatString(ESX_TEXT("sw {},0x{:04x}({})"), registersMnemonics[(U8)RegisterTarget()], Immediate(), registersMnemonics[(U8)RegisterSource()]);
					}
					case 0x2E: {
						return FormatString(ESX_TEXT("swr {},0x{:04x}({})"), registersMnemonics[(U8)RegisterTarget()], Immediate(), registersMnemonics[(U8)RegisterSource()]);
					}

					case 0x30:
					case 0x31:
					case 0x32:
					case 0x33: {
						U8 cpn = CO_N(binaryInstruction);
						return FormatString(ESX_TEXT("lwc{} ${},0x{:04x}({})"), cpn, RegisterTarget().Value, Immediate(), registersMnemonics[(U8)RegisterSource()]);
						break;
					}


					case 0x38:
					case 0x39:
					case 0x3A:
					case 0x3B: {
						U8 cpn = CO_N(binaryInstruction);
						return FormatString(ESX_TEXT("swc{} ${},0x{:04x}({})"), cpn, RegisterTarget().Value, Immediate(), registersMnemonics[(U8)RegisterSource()]);
						break;
					}
				}
			}
		}

		return ESX_TEXT("");
	}

}