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
#include "Core/SPU.h"

#include "optick.h"

namespace esx {

	R3000::R3000()
		:	BusDevice(ESX_TEXT("R3000"))
	{
		reset();

	}

	R3000::~R3000()
	{
	}

	void R3000::init()
	{

		mRootBus = getBus(ESX_TEXT("Root"));
		mTimer = getBus("Root")->getDevice<Timer>("Timer");
		mCDROM = getBus("Root")->getDevice<CDROM>("CDROM");
		mGPU = getBus("Root")->getDevice<GPU>("GPU");
		mSIO0 = getBus("Root")->getDevice<SIO>("SIO0");
		mSIO1 = getBus("Root")->getDevice<SIO>("SIO1");
		mInterruptControl = getBus("Root")->getDevice<InterruptControl>("InterruptControl");
		mDMA = getBus("Root")->getDevice<DMA>("DMA");
		mSPU = getBus("Root")->getDevice<SPU>("SPU");
	}

	void R3000::clock()
	{
		if (mCyclesToWait == 0) {
			if (!mStall) {
				U32 opcode = fetch(mPC);

				if (opcode != 0) {
					decode(mCurrentInstruction, opcode, mPC);
				}

				mCurrentPC = mPC;
				mPC = mNextPC;
				mNextPC += 4;

				mBranchSlot = mBranch;
				mTookBranchSlot = mTookBranch;
				mBranch = ESX_FALSE;
				mTookBranch = ESX_FALSE;

				if (opcode != 0 && mCurrentInstruction.Execute) {
					(this->*mCurrentInstruction.Execute)();
				}

				mRegisters[mMemoryLoad.first] = mMemoryLoad.second;
				mRegisters[0] = 0;
				mMemoryLoad = mPendingLoad;
				resetPendingLoad();

				mRegisters[mWriteBack.first] = mWriteBack.second;
				mWriteBack.first = RegisterIndex(0);
				mRegisters[0] = 0;


				switch (mCurrentPC) {
					case 0xA0: {
						BiosA0(mCallPC);
						break;
					}
					case 0xB0: {
						BiosB0(mCallPC);
						break;
					}
					case 0xC0: {
						BiosC0(mCallPC);
						break;
					}
				}
			}
			mCyclesToWait = 2;
		}
		mCyclesToWait--;

		mSIO0->clock(mCycles);
		if (!mDMA->isRunning()) {
			mStall = ESX_FALSE;
		}

		mInterruptControl->clock(mCycles);
		handleInterrupts();

		mCycles++;
	}


	U32 R3000::fetch(U32 address)
	{
		if (ADDRESS_UNALIGNED(address, U32)) {
			raiseException(ExceptionType::AddressErrorLoad);
			return 0;
		}

		if (isCacheActive(address)) {
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
			return mRootBus->load<U32>(address);
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

	static const Array<ExecuteFunction, 18> branchOpCodeDecode = {
		&R3000::BLTZ,
		&R3000::BGEZ,
		&R3000::BLTZ,
		&R3000::BGEZ,
		&R3000::BLTZ,
		&R3000::BGEZ,
		&R3000::BLTZ,
		&R3000::BGEZ,
		&R3000::BLTZ,
		&R3000::BGEZ,
		&R3000::BLTZ,
		&R3000::BGEZ,
		&R3000::BLTZ,
		&R3000::BGEZ,
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

		switch (primaryOpCode) {
			case 0x00: {
				if (secondaryOpCode < secondaryOpCodeDecode.size()) {
					result.Execute = secondaryOpCodeDecode[secondaryOpCode];
				}
				break;
			}

			case 0x01: {
				if (result.RegisterTarget().Value < branchOpCodeDecode.size()) {
					result.Execute = branchOpCodeDecode[result.RegisterTarget().Value];
				}
				break;
			}

			default: {
				if (primaryOpCode < primaryOpCodeDecode.size()) {
					result.Execute = primaryOpCodeDecode[primaryOpCode];
				}
			}
		}
	}

	void R3000::reset()
	{
		mRegisters = {};
		mCP0Registers = {};
		mGTE.reset();

		mPendingLoad = {};
		mMemoryLoad = {};
		mWriteBack = {};

		mPC = 0;
		mNextPC = 0;
		mCurrentPC = 0;
		mCallPC = 0;
		mHI = 0;
		mLO = 0;
		mICache = {};
		mStall = ESX_FALSE;

		mBranch = ESX_FALSE;
		mBranchSlot = ESX_FALSE;
		mTookBranch = ESX_FALSE;
		mTookBranchSlot = ESX_FALSE;

		mGPUClock = 0;

		mCycles = 0;

		mPC = 0xBFC00000;
		mNextPC = mPC + 4;
		setCP0Register(COP0Register::PRId, 0x00000002);
		resetPendingLoad();

		mMemoryLoad = std::make_pair<RegisterIndex, U32>(RegisterIndex(0), 0);
		mPendingLoad = std::make_pair<RegisterIndex, U32>(RegisterIndex(0), 0);
		mWriteBack = std::make_pair<RegisterIndex, U32>(RegisterIndex(0), 0);

		mTTY = {};
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
		I64 a = static_cast<I64>(static_cast<I32>(getRegister(mCurrentInstruction.RegisterSource())));
		I64 b = static_cast<I64>(static_cast<I32>(getRegister(mCurrentInstruction.RegisterTarget())));

		U64 r = static_cast<U64>(a * b);

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
		setRegister(mCurrentInstruction.RegisterDestination(), mLO);
	}

	void R3000::MTLO()
	{
		mLO = getRegister(mCurrentInstruction.RegisterSource());
	}

	void R3000::MFHI()
	{
		setRegister(mCurrentInstruction.RegisterDestination(), mHI);
	}

	void R3000::MTHI()
	{
		mHI = getRegister(mCurrentInstruction.RegisterSource());
	}

	void R3000::LW()
	{
		BIT exception = ESX_FALSE;

		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated LW from {:08x} not handled", m);
			return;
		}

		U32 r = load<U32>(m, exception);
		if (exception) {
			r = getRegister(mCurrentInstruction.RegisterTarget());
		}

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
	}

	void R3000::LH()
	{
		BIT exception = ESX_FALSE;

		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated LH from {:08x} not handled", m);
			return;
		}

		U32 r = load<U16>(m, exception);
		if (exception) {
			r = getRegister(mCurrentInstruction.RegisterTarget());
		} else {
			r = static_cast<U32>(static_cast<I32>(static_cast<I16>(static_cast<U16>(r))));
		}

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
	}

	void R3000::LHU()
	{
		BIT exception = ESX_FALSE;

		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated LHU from {:08x} not handled", m);
			return;
		}

		U32 r = load<U16>(m, exception);
		if (exception) {
			r = getRegister(mCurrentInstruction.RegisterTarget());
		}

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
	}

	void R3000::LB()
	{
		BIT exception = ESX_FALSE;

		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated LB from {:08x} not handled", m);
			return;
		}

		U32 r = load<U8>(m, exception);
		r = static_cast<U32>(static_cast<I32>(static_cast<I8>(static_cast<U8>(r))));

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
	}

	void R3000::LBU()
	{
		BIT exception = ESX_FALSE;

		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated LBU from {:08x} not handled", m);
			return;
		}

		U32 r = load<U8>(m, exception);

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
	}

	void R3000::LWL()
	{
		BIT exception = ESX_FALSE;

		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();
		U32 c = getRegister(mCurrentInstruction.RegisterTarget());
		if (mMemoryLoad.first == mCurrentInstruction.RegisterTarget()) {
			c = mMemoryLoad.second;
		}

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated LWL from {:08x} not handled", m);
			return;
		}

		U32 am = m & ~(0x3);
		U32 aw = load<U32>(am, exception);

		U32 u = m & 0x3;
		U32 r = (c & (0x00FFFFFF >> (u * 8))) | (aw << (24 - (u * 8)));

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
	}

	void R3000::SWL()
	{
		BIT exception = ESX_FALSE;

		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();
		U32 c = getRegister(mCurrentInstruction.RegisterTarget());

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated SWL from {:08x} not handled", m);
			return;
		}

		U32 am = m & ~(0x3);
		U32 aw = load<U32>(am, exception);

		U32 u = (m & 0x3) * 8;
		U32 mr = (aw & (0xFFFFFF00 << u)) | (c >> (24 - u));

		store<U32>(am, mr);
	}


	void R3000::LWR()
	{
		BIT exception = ESX_FALSE;

		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();
		U32 c = getRegister(mCurrentInstruction.RegisterTarget());
		if (mMemoryLoad.first == mCurrentInstruction.RegisterTarget()) {
			c = mMemoryLoad.second;
		}

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated LWR from {:08x} not handled", m);
			return;
		}

		U32 am = m & ~(0x3);
		U32 aw = load<U32>(am, exception);

		U32 u = m & 0x3;
		U32 r = (c & (0xFFFFFF00 << ((0x3 - u) * 8))) | (aw >> (u * 8));

		addPendingLoad(mCurrentInstruction.RegisterTarget(), r);
	}

	void R3000::SWR()
	{
		BIT exception = ESX_FALSE;

		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();
		U32 c = getRegister(mCurrentInstruction.RegisterTarget());

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated SWR to {:08x} not handled", m);
			return;
		}

		U32 am = m & ~(0x3);
		U32 aw = load<U32>(am, exception);

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
			ESX_CORE_LOG_WARNING("Cache SB store to {:08x} not handled", m);
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
			ESX_CORE_LOG_WARNING("Cache SH store to {:08x} not handled", m);
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

		U32 v = getRegister(mCurrentInstruction.RegisterTarget());

		if ((sr & 0x10000) != 0) {
			iCacheStore(m, v);
			return;
		}


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
		//ESX_CORE_LOG_TRACE("BEQ {:08x}h", mCurrentInstruction.Address);

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

		setRegister(GPRRegister::ra, mNextPC);

		if (a < 0) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}

		ESX_CORE_LOG_TRACE("{:08x}h", mCurrentInstruction.Address);
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

		setRegister(GPRRegister::ra, mNextPC);

		if (a >= 0) {
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
		mCallPC = mCurrentPC;
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
		setRegister(mCurrentInstruction.RegisterDestination(), mNextPC);
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
		//ESX_CORE_LOG_TRACE("COP2");
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
			mGTE.command(mCurrentInstruction.Immediate25());
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
		addPendingLoad(mCurrentInstruction.RegisterTarget(), mGTE.getRegister(32 + mCurrentInstruction.RegisterDestination().Value));
	}

	void R3000::MTC2()
	{
		mGTE.setRegister(mCurrentInstruction.RegisterDestination().Value, getRegister(mCurrentInstruction.RegisterTarget()));
	}

	void R3000::MFC2()
	{
		addPendingLoad(mCurrentInstruction.RegisterTarget(), mGTE.getRegister(mCurrentInstruction.RegisterDestination().Value));
	}

	void R3000::CTC2()
	{
		mGTE.setRegister(32 + mCurrentInstruction.RegisterDestination().Value, getRegister(mCurrentInstruction.RegisterTarget()));
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
		mBranch = ESX_TRUE;
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		if (mGTE.getFlag() == false) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
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
		mBranch = ESX_TRUE;
		I32 o = mCurrentInstruction.ImmediateSE() << 2;

		if (mGTE.getFlag() == true) {
			mNextPC += o;
			mNextPC -= 4;
			mTookBranch = ESX_TRUE;
		}
	}

	void R3000::RFE()
	{
		U32 sr = getCP0Register(COP0Register::SR);

		if ((sr & 0x10000002) == 0x1) {
			raiseException(ExceptionType::CoprocessorUnusable);
			return;
		}

		U32 mode = sr & 0x3F;
		sr &= ~0xF;
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
		BIT exception = ESX_FALSE;

		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated load from {:08x} not handled", m);
			return;
		}

		U32 r = load<U32>(m, exception);

		mGTE.setRegister(mCurrentInstruction.RegisterTarget().Value, r);
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
		U32 sr = getCP0Register(COP0Register::SR);
		U32 a = getRegister(mCurrentInstruction.RegisterSource());
		U32 b = mCurrentInstruction.ImmediateSE();

		U32 m = a + b;

		if ((sr & 0x10000) != 0) {
			ESX_CORE_LOG_WARNING("Cache isolated store to {:08x} not handled", m);
			return;
		}

		U32 v = mGTE.getRegister(mCurrentInstruction.RegisterTarget().Value);

		store<U32>(m, v);
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
		mWriteBack.first = index;
		mWriteBack.second = value;
	}

	U32 R3000::getRegister(RegisterIndex index)
	{
		return mRegisters[index.Value];
	}

	void R3000::BiosPutChar(char c)
	{
		mTTY << c;
	}

	void R3000::BiosPuts(U32 src)
	{
		BIT exception = ESX_FALSE;
		while (true) {
			U8 c = load<U8>(src, exception);
			if (c == '\0') {
				break;
			}
			BiosPutChar(c);
			src++;
		};
	}

	void R3000::BiosWrite(U32 fd, U32 src, U32 length)
	{
		BIT exception = ESX_FALSE;
		if (fd == 1) {
			for (I32 i = 0; i < length; i++) {
				U8 c = load<U8>(src + i, exception);
				BiosPutChar(c);
			}
		}
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
		auto& cacheLine = mICache.CacheLines[cacheLineNumber];
		cacheLine.Tag = tag;

		for (U32 index = 0; index < 4; index++) {
			auto& instruction = cacheLine.Instructions[index];

			if (index >= startIndex) {
				U32 word = mRootBus->load<U32>(address + (index - startIndex) * 4);
				instruction.Word = word;
				instruction.Valid = ESX_TRUE;
			} else {
				instruction.Valid = ESX_FALSE;
			}
		}

		return cacheLine.Instructions[startIndex].Word;
	}

	void R3000::BiosA0(U32 callPC)
	{
		switch (mRegisters[9]) {
			case 0x00: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - open(filename,accessmode)",callPC); break;
			case 0x01: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - lseek(fd,offset,seektype)",callPC); break;
			case 0x02: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - read(fd,dst,length)",callPC); break;
			case 0x03: BiosWrite(mRegisters[4], mRegisters[5], mRegisters[6]); break;//case 0x03: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - write(fd,src,length)",callPC); break;
			case 0x04: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - close(fd)",callPC); break;
			case 0x05: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - ioctl(fd,cmd,arg)",callPC); break;
			case 0x06: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - exit(exitcode)",callPC); break;
			case 0x07: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - isatty(fd)",callPC); break;
			case 0x08: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - getc(fd)",callPC); break;
			case 0x09: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - putc(char,fd)",callPC); break;
			case 0x0A: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - todigit(char)",callPC); break;
			case 0x0B: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - atof(src)",callPC); break;
			case 0x0C: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - strtoul(src,src_end,base)",callPC); break;
			case 0x0D: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - strtol(src,src_end,base)",callPC); break;
			case 0x0E: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - abs(val)",callPC); break;
			case 0x0F: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - labs(val)",callPC); break;
			case 0x10: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - atoi(src)",callPC); break;
			case 0x11: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - atol(src)",callPC); break;
			case 0x12: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - atob(src,num_dst)",callPC); break;
			case 0x13: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - setjmp(buf)",callPC); break;
			case 0x14: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - longjmp(buf,param)",callPC); break;
			case 0x15: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - strcat(dst,src)",callPC); break;
			case 0x16: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - strncat(dst,src,maxlen)",callPC); break;
			case 0x17: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - strcmp(str1,str2)",callPC); break;
			case 0x18: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - strncmp(str1,str2,maxlen)",callPC); break;
			case 0x19: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - strcpy(dst,src)",callPC); break;
			case 0x1A: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - strncpy(dst,src,maxlen)",callPC); break;
			case 0x1B: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - strlen(src)",callPC); break;
			case 0x1C: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - index(src,char)",callPC); break;
			case 0x1D: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - rindex(src,char)",callPC); break;
			case 0x1E: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - strchr(src,char)",callPC); break;
			case 0x1F: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - strrchr(src,char)",callPC); break;
			case 0x20: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - strpbrk(src,list)",callPC); break;
			case 0x21: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - strspn(src,list)",callPC); break;
			case 0x22: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - strcspn(src,list)",callPC); break;
			case 0x23: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - strtok(src,list)",callPC); break;
			case 0x24: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - strstr(str,substr)",callPC); break;
			case 0x25: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - toupper(char)",callPC); break;
			case 0x26: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - tolower(char)",callPC); break;
			case 0x27: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - bcopy(src,dst,len)",callPC); break;
			case 0x28: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - bzero(dst,len)",callPC); break;
			case 0x29: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - bcmp(ptr1,ptr2,len)",callPC); break;
			case 0x2A: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - memcpy(dst,src,len)",callPC); break;
			case 0x2B: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - memset(dst,fillbyte,len)",callPC); break;
			case 0x2C: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - memmove(dst,src,len)",callPC); break;
			case 0x2D: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - memcmp(src1,src2,len)",callPC); break;
			case 0x2E: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - memchr(src,scanbyte,len)",callPC); break;
			case 0x2F: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - rand()",callPC); break;
			case 0x30: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - srand(seed)",callPC); break;
			case 0x31: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - qsort(base,nel,width,callback)",callPC); break;
			case 0x32: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - strtod(src,src_end)",callPC); break;
			case 0x33: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - malloc(size)",callPC); break;
			case 0x34: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - free(buf)",callPC); break;
			case 0x35: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - lsearch(key,base,nel,width,callback)",callPC); break;
			case 0x36: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - bsearch(key,base,nel,width,callback)",callPC); break;
			case 0x37: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - calloc(sizx,sizy)",callPC); break;
			case 0x38: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - realloc(old_buf,new_siz)",callPC); break;
			case 0x39: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - InitHeap(addr,size)",callPC); break;
			case 0x3A: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - _exit(exitcode)",callPC); break;
			case 0x3B: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - getchar()",callPC); break;
			case 0x3C: BiosPutChar((char)mRegisters[4]); break; //ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - putchar('{}')",callPC,(char)mRegisters[4]); break;
			case 0x3D: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - gets(dst)",callPC); break;
			case 0x3E: BiosPuts(mRegisters[4]); break;// ESX_CORE_LOG_TRACE("0x{:08X} - puts(src)", callPC); break;
			case 0x3F: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - printf(txt,param1,param2,etc.)",callPC); break;
			case 0x40: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - SystemErrorUnresolvedException()",callPC); break;
			case 0x41: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - LoadTest(filename,headerbuf)",callPC); break;
			case 0x42: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - Load(filename,headerbuf)",callPC); break;
			case 0x43: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - Exec(headerbuf,param1,param2)",callPC); break;
			case 0x44: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - FlushCache()",callPC); break;
			case 0x45: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - init_a0_b0_c0_vectors",callPC); break;
			case 0x46: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - GPU_dw(Xdst,Ydst,Xsiz,Ysiz,src)",callPC); break;
			case 0x47: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - gpu_send_dma(Xdst,Ydst,Xsiz,Ysiz,src)",callPC); break;
			case 0x48: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - SendGP1Command(gp1cmd)",callPC); break;
			case 0x49: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - GPU_cw(gp0cmd)",callPC); break;
			case 0x4A: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - GPU_cwp(src,num)",callPC); break;
			case 0x4B: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - send_gpu_linked_list(src)",callPC); break;
			case 0x4C: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - gpu_abort_dma()",callPC); break;
			case 0x4D: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - GetGPUStatus()",callPC); break;
			case 0x4E: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - gpu_sync()",callPC); break;
			case 0x4F: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - SystemError",callPC); break;
			case 0x50: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - SystemError",callPC); break;
			case 0x51: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - LoadExec(filename,stackbase,stackoffset)",callPC); break;
			case 0x52: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - GetSysSp",callPC); break;
			case 0x53: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - SystemError",callPC); break;
			case 0x54: case 0x71: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - _96_init()",callPC); break;
			case 0x55: case 0x70: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - _bu_init()",callPC); break;
			case 0x56: case 0x72: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - _96_remove()",callPC); break;
			case 0x57: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x58: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x59: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x5A: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x5B: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - dev_tty_init()",callPC); break;
			case 0x5C: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - dev_tty_open(fcb,and unused:'path\name', accessmode)",callPC); break;
			case 0x5D: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - dev_tty_in_out(fcb,cmd)",callPC); break;
			case 0x5E: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - dev_tty_ioctl(fcb,cmd,arg)",callPC); break;
			case 0x5F: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - dev_cd_open(fcb,'path\name',accessmode)",callPC); break;
			case 0x60: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - dev_cd_read(fcb,dst,len)",callPC); break;
			case 0x61: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - dev_cd_close(fcb)",callPC); break;
			case 0x62: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - dev_cd_firstfile(fcb,'path\name',direntry)",callPC); break;
			case 0x63: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - dev_cd_nextfile(fcb,direntry)",callPC); break;
			case 0x64: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - dev_cd_chdir(fcb,'path')",callPC); break;
			case 0x65: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - dev_card_open(fcb,'path\name',accessmode)",callPC); break;
			case 0x66: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - dev_card_read(fcb,dst,len)",callPC); break;
			case 0x67: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - dev_card_write(fcb,src,len)",callPC); break;
			case 0x68: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - dev_card_close(fcb)",callPC); break;
			case 0x69: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - dev_card_firstfile(fcb,'path\name',direntry)",callPC); break;
			case 0x6A: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - dev_card_nextfile(fcb,direntry)",callPC); break;
			case 0x6B: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - dev_card_erase(fcb,'path\name')",callPC); break;
			case 0x6C: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - dev_card_undelete(fcb,'path\name')",callPC); break;
			case 0x6D: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - dev_card_format(fcb)",callPC); break;
			case 0x6E: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - dev_card_rename(fcb1,'path\name1',fcb2,'path\name2')",callPC); break;
			case 0x6F: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - ? ;card ;[r4+18h]=00000000h",callPC); break;
			case 0x73: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x74: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x75: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x76: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x77: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x78: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - CdAsyncSeekL(src)",callPC); break;
			case 0x79: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x7A: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x7B: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x7C: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - CdAsyncGetStatus(dst)",callPC); break;
			case 0x7D: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x7E: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - CdAsyncReadSector(count,dst,mode)",callPC); break;
			case 0x7F: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x80: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x81: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - CdAsyncSetMode(mode)",callPC); break;
			case 0x82: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x83: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x84: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x85: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x86: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x87: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x88: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x89: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x8A: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x8B: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x8C: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x8D: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x8E: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x8F: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0",callPC); break;
			case 0x90: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - CdromIoIrqFunc1()",callPC); break;
			case 0x91: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - CdromDmaIrqFunc1()",callPC); break;
			case 0x92: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - CdromIoIrqFunc2()",callPC); break;
			case 0x93: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - CdromDmaIrqFunc2()",callPC); break;
			case 0x94: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - CdromGetInt5errCode(dst1,dst2)",callPC); break;
			case 0x95: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - CdInitSubFunc()",callPC); break;
			case 0x96: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - AddCDROMDevice()",callPC); break;
			case 0x97: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - AddMemCardDevice()",callPC); break;
			case 0x98: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - AddDuartTtyDevice()",callPC); break;
			case 0x99: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - add_nullcon_driver()",callPC); break;
			case 0x9A: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - SystemError",callPC); break;
			case 0x9B: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - SystemError",callPC); break;
			case 0x9C: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - SetConf(num_EvCB,num_TCB,stacktop)",callPC); break;
			case 0x9D: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - GetConf(num_EvCB_dst,num_TCB_dst,stacktop_dst)",callPC); break;
			case 0x9E: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - SetCdromIrqAutoAbort(type,flag)",callPC); break;
			case 0x9F: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - SetMem(megabytes)",callPC); break;
			case 0xA0: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - _boot()", callPC); break;
			case 0xA1: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - SystemError({},{})", callPC, (Char)mRegisters[4], (I32)mRegisters[5]); break;
			case 0xA2: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - EnqueueCdIntr()", callPC); break;
			case 0xA3: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - DequeueCdIntr()", callPC); break;
			case 0xA4: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - CdGetLbn(filename)", callPC); break;
			case 0xA5: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - CdReadSector(count,sector,buffer)", callPC); break;
			case 0xA6: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - CdGetStatus()", callPC); break;
			case 0xA7: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - bufs_cb_0()", callPC); break;
			case 0xA8: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - bufs_cb_1()", callPC); break;
			case 0xA9: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - bufs_cb_2()", callPC); break;
			case 0xAA: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - bufs_cb_3()", callPC); break;
			case 0xAB: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - _card_info(port)", callPC); break;
			case 0xAC: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - _card_load(port)", callPC); break;
			case 0xAD: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - _card_auto(flag)", callPC); break;
			case 0xAE: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - bufs_cb_4()", callPC); break;
			case 0xAF: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - card_write_test(port)", callPC); break;
			case 0xB0: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0", callPC); break;
			case 0xB1: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0", callPC); break;
			case 0xB2: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - ioabort_raw(param)", callPC); break;
			case 0xB3: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - return 0", callPC); break;
			case 0xB4: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - GetSystemInfo(index)", callPC); break;
		}
	}

	void R3000::BiosB0(U32 callPC)
	{
		switch (mRegisters[9]) {
			case 0x00: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - alloc_kernel_memory(size)", callPC); break;
			case 0x01: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - free_kernel_memory(buf)", callPC); break;
			case 0x02: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - init_timer(t,reload,flags)", callPC); break;
			case 0x03: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - get_timer(t)", callPC); break;
			case 0x04: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - enable_timer_irq(t)", callPC); break;
			case 0x05: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - disable_timer_irq(t)", callPC); break;
			case 0x06: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - restart_timer(t)", callPC); break;
			case 0x07: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - DeliverEvent(class, spec)", callPC); break;
			case 0x08: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - OpenEvent(class,spec,mode,func)", callPC); break;
			case 0x09: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - CloseEvent(event)", callPC); break;
			case 0x0A: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - WaitEvent(event)", callPC); break;
			case 0x0B: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - TestEvent(event)", callPC); break;
			case 0x0C: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - EnableEvent(event)", callPC); break;
			case 0x0D: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - DisableEvent(event)", callPC); break;
			case 0x0E: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - OpenTh(reg_PC,reg_SP_FP,reg_GP)", callPC); break;
			case 0x0F: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - CloseTh(handle)", callPC); break;
			case 0x10: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - ChangeTh(handle)", callPC); break;
			case 0x11: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - jump_to_00000000h", callPC); break;
			case 0x12: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - InitPAD2(buf1,siz1,buf2,siz2)", callPC); break;
			case 0x13: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - StartPAD2()", callPC); break;
			case 0x14: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - StopPAD2()", callPC); break;
			case 0x15: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - PAD_init2(type,button_dest,unused,unused)", callPC); break;
			case 0x16: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - PAD_dr()", callPC); break;
			case 0x17: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - ReturnFromException()", callPC); break;
			case 0x18: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - ResetEntryInt()", callPC); break;
			case 0x19: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - HookEntryInt(addr)", callPC); break;
			case 0x1A: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - SystemError  ", callPC); break;
			case 0x1B: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - SystemError  ", callPC); break;
			case 0x1C: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - SystemError  ", callPC); break;
			case 0x1D: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - SystemError  ", callPC); break;
			case 0x1E: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - SystemError  ", callPC); break;
			case 0x1F: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - SystemError  ", callPC); break;
			case 0x20: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - UnDeliverEvent(class,spec)", callPC); break;
			case 0x21: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - SystemError  ", callPC); break;
			case 0x22: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - SystemError  ", callPC); break;
			case 0x23: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - SystemError  ", callPC); break;
			case 0x24: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - jump_to_00000000h", callPC); break;
			case 0x25: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - jump_to_00000000h", callPC); break;
			case 0x26: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - jump_to_00000000h", callPC); break;
			case 0x27: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - jump_to_00000000h", callPC); break;
			case 0x28: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - jump_to_00000000h", callPC); break;
			case 0x29: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - jump_to_00000000h", callPC); break;
			case 0x2A: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - SystemError  ", callPC); break;
			case 0x2B: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - SystemError  ", callPC); break;
			case 0x2C: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - jump_to_00000000h", callPC); break;
			case 0x2D: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - jump_to_00000000h", callPC); break;
			case 0x2E: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - jump_to_00000000h", callPC); break;
			case 0x2F: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - jump_to_00000000h", callPC); break;
			case 0x30: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - jump_to_00000000h", callPC); break;
			case 0x31: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - jump_to_00000000h", callPC); break;
			case 0x32: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - open(filename,accessmode)", callPC); break;
			case 0x33: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - lseek(fd,offset,seektype)", callPC); break;
			case 0x34: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - read(fd,dst,length)", callPC); break;
			case 0x35: BiosWrite(mRegisters[4], mRegisters[5], mRegisters[6]); break; //ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - write(fd,src,length)", callPC); break;
			case 0x36: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - close(fd)", callPC); break;
			case 0x37: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - ioctl(fd,cmd,arg)", callPC); break;
			case 0x38: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - exit(exitcode)", callPC); break;
			case 0x39: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - isatty(fd)", callPC); break;
			case 0x3A: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - getc(fd)", callPC); break;
			case 0x3B: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - putc(char,fd)", callPC); break;
			case 0x3C: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - getchar()", callPC); break;
			case 0x3D: BiosPutChar((char)mRegisters[4]); break; //ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - putchar('{}')",callPC,(char)mRegisters[4]); break;
			case 0x3E: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - gets(dst)", callPC); break;
			case 0x3F: BiosPuts(mRegisters[4]); break;// ESX_CORE_LOG_TRACE("0x{:08X} - puts(src)", callPC); break;
			case 0x40: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - cd(name)", callPC); break;
			case 0x41: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - format(devicename)", callPC); break;
			case 0x42: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - firstfile2(filename,direntry)", callPC); break;
			case 0x43: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - nextfile(direntry)", callPC); break;
			case 0x44: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - rename(old_filename,new_filename)", callPC); break;
			case 0x45: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - erase(filename)", callPC); break;
			case 0x46: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - undelete(filename)", callPC); break;
			case 0x47: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - AddDrv(device_info)  ", callPC); break;
			case 0x48: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - DelDrv(device_name_lowercase)", callPC); break;
			case 0x49: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - PrintInstalledDevices()", callPC); break;
			case 0x4A: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - InitCARD2(pad_enable)", callPC); break;
			case 0x4B: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - StartCARD2()", callPC); break;
			case 0x4C: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - StopCARD2()", callPC); break;
			case 0x4D: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - _card_info_subfunc(port)", callPC); break;
			case 0x4E: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - _card_write(port,sector,src)", callPC); break;
			case 0x4F: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - _card_read(port,sector,dst)", callPC); break;
			case 0x50: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - _new_card()", callPC); break;
			case 0x51: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - Krom2RawAdd(shiftjis_code)", callPC); break;
			case 0x52: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - SystemError ", callPC); break;
			case 0x53: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - Krom2Offset(shiftjis_code)", callPC); break;
			case 0x54: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - _get_errno()", callPC); break;
			case 0x55: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - _get_error(fd)", callPC); break;
			case 0x56: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - GetC0Table", callPC); break;
			case 0x57: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - GetB0Table", callPC); break;
			case 0x58: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - _card_chan()", callPC); break;
			case 0x59: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - testdevice(devicename)", callPC); break;
			case 0x5A: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - SystemError  ", callPC); break;
			case 0x5B: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - ChangeClearPAD(int)", callPC); break;
			case 0x5C: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - _card_status(slot)", callPC); break;
			case 0x5D: ESX_CORE_BIOS_LOG_TRACE("0x{:08X} - _card_wait(slot)", callPC); break;
		}
	}

	void R3000::BiosC0(U32 callPC)
	{
	}

	void R3000::iCacheStore(U32 address, U32 value)
	{
		U32 cacheLineNumber = address / 16;
		U32 wordAddress = (address & 0xF) / 4;

		InstructionCache& instruction = mICache.CacheLines[cacheLineNumber].Instructions[wordAddress];
		instruction.Word = value;
		instruction.Valid = ESX_FALSE;
	}

	void R3000::addWriteQueueOperation(const StoreOperation& writeOp)
	{
		if (writeOp.Address == 0x80) {
			ESX_CORE_LOG_ERROR("{:08x}h", mCurrentInstruction.Address);
		}
		mWriteQueue.push_back(writeOp);
	}

	void R3000::doWriteQueueOperation(const StoreOperation& writeOp)
	{
		switch (writeOp.Size) {
			case sizeof(U8)  :  mRootBus->store<U8> (writeOp.Address, static_cast<U8> (writeOp.Data)); break;
			case sizeof(U16) :	mRootBus->store<U16>(writeOp.Address, static_cast<U16>(writeOp.Data)); break;
			case sizeof(U32) :	mRootBus->store<U32>(writeOp.Address, static_cast<U32>(writeOp.Data)); break;
		}
	}

	BIT R3000::flushWriteQueue(U32 address)
	{
		auto foundIt = std::find_if(mWriteQueue.begin(), mWriteQueue.end(), [&](const StoreOperation& storeOp) { return storeOp.Address == address; });
		if (foundIt != mWriteQueue.end()) {
			doWriteQueueOperation(*foundIt);
			mWriteQueue.erase(foundIt);
			return ESX_TRUE;
		}
		return ESX_FALSE;
	}

	void R3000::flushWriteQueueFirst()
	{
		if (mWriteQueue.size() == 0) return;
		auto it = mWriteQueue.begin();
		doWriteQueueOperation(*it);
		mWriteQueue.erase(it);
	}

	void R3000::flushWriteQueueAll()
	{
		for (auto it = mWriteQueue.begin(); it != mWriteQueue.end(); ++it) {
			doWriteQueueOperation(*it);
		}
		mWriteQueue.clear();
	}

	void R3000::handleInterrupts()
	{
		U32 cause = getCP0Register(COP0Register::Cause);
		U32 sr = getCP0Register(COP0Register::SR);

		U32 opcodeNextInstruction = fetch(mPC);
		if ((opcodeNextInstruction >> 26) != 0x12) {
			if (mInterruptControl->interruptPending()) {
				cause |= (1 << 10);
			}
			else {
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

	String Instruction::Mnemonic(const SharedPtr<R3000>& cpuState) const
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
						return FormatString(ESX_TEXT("lb {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}
					case 0x21: {
						return FormatString(ESX_TEXT("lh {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}
					case 0x22: {
						return FormatString(ESX_TEXT("lwl {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}
					case 0x23: {
						return FormatString(ESX_TEXT("lw {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}
					case 0x24: {
						return FormatString(ESX_TEXT("lbu {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}
					case 0x25: {
						return FormatString(ESX_TEXT("lhu {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}
					case 0x26: {
						return FormatString(ESX_TEXT("lwr {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}
					case 0x28: {
						return FormatString(ESX_TEXT("sb {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}
					case 0x29: {
						return FormatString(ESX_TEXT("sh {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}
					case 0x2A: {
						return FormatString(ESX_TEXT("swl {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}
					case 0x2B: {
						return FormatString(ESX_TEXT("sw {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}
					case 0x2E: {
						return FormatString(ESX_TEXT("swr {},{}({}) [0x{:08x}]"), registersMnemonics[(U8)RegisterTarget()], (I32)ImmediateSE(), registersMnemonics[(U8)RegisterSource()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
					}

					case 0x30:
					case 0x31:
					case 0x32:
					case 0x33: {
						U8 cpn = CO_N(binaryInstruction);
						return FormatString(ESX_TEXT("lwc{} ${},0x{:08x}"), cpn, registersMnemonics[(U8)RegisterTarget()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
						break;
					}


					case 0x38:
					case 0x39:
					case 0x3A:
					case 0x3B: {
						U8 cpn = CO_N(binaryInstruction);
						return FormatString(ESX_TEXT("swc{} ${},0x{:08x}"), cpn, registersMnemonics[(U8)RegisterTarget()], cpuState->getRegister(RegisterSource()) + ImmediateSE());
						break;
					}
				}
			}
		}

		return ESX_TEXT("");
	}

}