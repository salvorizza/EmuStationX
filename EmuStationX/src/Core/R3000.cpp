#include "R3000.h"

#include <iostream>
#include <iomanip>
#include <fstream>

#include "Core/GPU.h"
#include "Core/Timer.h"
#include "Core/CDROM.h"
#include "Core/SIO.h"
#include "Core/InterruptControl.h"

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

		U32 opcode = fetch(mPC);

		decode(mCurrentInstruction,opcode, mPC);

		ESX_CORE_ASSERT(mCurrentInstruction.Execute, "No Operation");

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

		mRegisters = mOutRegisters;

		mGPU->clock();
		mTimer->systemClock();
		mCDROM->clock();
		mSIO0->clock();

		handleInterrupts();

		mCycles++;
	}


	U32 R3000::fetch(U32 address)
	{
		return load<U32>(address);
	}

	void R3000::decode(Instruction& result, U32 instruction, U32 address, BIT suppressException)
	{
		result.Address = address;
		result.binaryInstruction = instruction;
		result.Execute = nullptr;

		switch (result.Opcode()) {
			//R Type
			case 0x00: {
				switch (result.Function()) {
					case 0x00: {
						result.Execute = &R3000::SLL;
						break;
					}
					case 0x02: {
						result.Execute = &R3000::SRL;
						break;
					}
					case 0x03: {
						result.Execute = &R3000::SRA;
						break;
					}
					case 0x04: {
						result.Execute = &R3000::SLLV;
						break;
					}
					case 0x06: {
						result.Execute = &R3000::SRLV;
						break;
					}
					case 0x07: {
						result.Execute = &R3000::SRAV;
						break;
					}
					case 0x08: {
						result.Execute = &R3000::JR;
						break;
					}
					case 0x09: {
						result.Execute = &R3000::JALR;
						break;
					}
					case 0x0C: {
						result.Execute = &R3000::SYSCALL;
						break;
					}
					case 0x0D: {
						result.Execute = &R3000::BREAK;
						break;
					}
					case 0x10: {
						result.Execute = &R3000::MFHI;
						break;
					}
					case 0x11: {
						result.Execute = &R3000::MTHI;
						break;
					}
					case 0x12: {
						result.Execute = &R3000::MFLO;
						break;
					}
					case 0x13: {
						result.Execute = &R3000::MTLO;
						break;
					}
					case 0x18: {
						result.Execute = &R3000::MULT;
						break;
					}
					case 0x19: {
						result.Execute = &R3000::MULTU;
						break;
					}
					case 0x1A: {
						result.Execute = &R3000::DIV;
						break;
					}
					case 0x1B:{
						result.Execute = &R3000::DIVU;
						break;
					}
					case 0x20: {
						result.Execute = &R3000::ADD;
						break;
					}
					case 0x21: {
						result.Execute = &R3000::ADDU;
						break;
					}
					case 0x22: {
						result.Execute = &R3000::SUB;
						break;
					}
					case 0x23: {
						result.Execute = &R3000::SUBU;
						break;
					}
					case 0x24: {
						result.Execute = &R3000::AND;
						break;
					}
					case 0x25: {
						result.Execute = &R3000::OR;
						break;
					}
					case 0x26: {
						result.Execute = &R3000::XOR;
						break;
					}
					case 0x27: {
						result.Execute = &R3000::NOR;
						break;
					}
					case 0x2A: {
						result.Execute = &R3000::SLT;
						break;
					}
					case 0x2B: {
						result.Execute = &R3000::SLTU;
						break;
					}
					default: {
						if(!suppressException) raiseException(ExceptionType::ReservedInstruction);
						break;
					}
				}

				break;
			}

			//J Type
			case 0x02: {
				result.Execute = &R3000::J;
				break;
			}
			case 0x03: {
				result.Execute = &R3000::JAL;
				break;
			}

			default: {
				switch (result.Opcode()) {
					case 0x01: {
						switch (result.RegisterTarget().Value) {
							case 0x00: {
								result.Execute = &R3000::BLTZ;
								break;
							}
							case 0x01: {
								result.Execute = &R3000::BGEZ;
								break;
							}
							case 0x10: {
								result.Execute = &R3000::BLTZAL;
								break;
							}
							case 0x11: {
								result.Execute = &R3000::BGEZAL;
								break;
							}

							default: {
								if (!suppressException) raiseException(ExceptionType::ReservedInstruction);
								break;
							}
						}
						
						break;
					}
					case 0x04: {
						result.Execute = &R3000::BEQ;
						break;
					}
					case 0x05: {
						result.Execute = &R3000::BNE;
						break;
					}
					case 0x06: {
						result.Execute = &R3000::BLEZ;
						break;
					}
					case 0x07: {
						result.Execute = &R3000::BGTZ;
						break;
					}
					case 0x08: {
						result.Execute = &R3000::ADDI;
						break;
					}
					case 0x09: {
						result.Execute = &R3000::ADDIU;
						break;
					}
					case 0x0A: {
						result.Execute = &R3000::SLTI;
						break;
					}
					case 0x0B: {
						result.Execute = &R3000::SLTIU;
						break;
					}
					case 0x0C: {
						result.Execute = &R3000::ANDI;
						break;
					}
					case 0x0D: {
						result.Execute = &R3000::ORI;
						break;
					}
					case 0x0E: {
						result.Execute = &R3000::XORI;
						break;
					}
					case 0x0F: {
						result.Execute = &R3000::LUI;
						break;
					}
					case 0x10:
					case 0x11:
					case 0x12:
					case 0x13: {
						U8 cpn = CO_N(instruction);
						if (CO(instruction) == 0) {
							switch (result.RegisterSource().Value) {
								case 0x00: {
									switch (cpn) {
										case 0x0:
											result.Execute = &R3000::MFC0;
											break;
										case 0x2:
											result.Execute = &R3000::MFC2;
											break;
										default:
											if (!suppressException) raiseException(ExceptionType::CoprocessorUnusable);
											break;
									}
									break;
								}
								case 0x02: {
									switch (cpn) {
										case 0x2:
											result.Execute = &R3000::CFC2;
											break;
										default:
											if (!suppressException) raiseException(ExceptionType::CoprocessorUnusable);
											break;
									}
									break;
								}
								case 0x04: {
									switch (cpn) {
										case 0x0:
											result.Execute = &R3000::MTC0;
											break;
										case 0x2:
											result.Execute = &R3000::MTC2;
											break;
										default:
											if (!suppressException) raiseException(ExceptionType::CoprocessorUnusable);
											break;
									}
									break;
								}
								case 0x06: {
									switch (cpn) {
										case 0x2:
											result.Execute = &R3000::CTC2;
											break;
										default:
											if (!suppressException) raiseException(ExceptionType::CoprocessorUnusable);
											break;
									}
									break;
								}
								case 0x08: {
									switch (result.RegisterTarget()) {
										case 0x00: {
											switch (cpn) {
												case 0x0:
													result.Execute = &R3000::BC0F;
													break;
												case 0x2:
													result.Execute = &R3000::BC2F;
													break;
												default:
													if (!suppressException) raiseException(ExceptionType::CoprocessorUnusable);
													break;
											}
											break;
										}

										case 0x01: {
											switch (cpn) {
												case 0x0:
													result.Execute = &R3000::BC0T;
													break;
												case 0x2:
													result.Execute = &R3000::BC2T;
													break;
												default:
													if (!suppressException) raiseException(ExceptionType::CoprocessorUnusable);
													break;
											}
											break;
										}

										default: {
											if (!suppressException) raiseException(ExceptionType::ReservedInstruction);
											break;
										}
									}
									break;
								}
								default: {
									if (!suppressException) raiseException(ExceptionType::ReservedInstruction);
									break;
								}
							}
						}
						else {
							switch (cpn) {
								case 0: {
									switch (COP_FUNC(instruction)) {
										case 0x10: {
											result.Execute = &R3000::RFE;
											break;
										}

										default: {
											if (!suppressException) raiseException(ExceptionType::ReservedInstruction);
											break;
										}
									}
									break;
								}

								case 2: {
									result.Execute = &R3000::COP2;
									break;
								}

								default:
									if (!suppressException) raiseException(ExceptionType::CoprocessorUnusable);
									break;
							}
						}
						
						break;
					}
					case 0x20: {
						result.Execute = &R3000::LB;
						break;
					}
					case 0x21: {
						result.Execute = &R3000::LH;
						break;
					}
					case 0x22: {
						result.Execute = &R3000::LWL;
						break;
					}
					case 0x23: {
						result.Execute = &R3000::LW;
						break;
					}
					case 0x24: {
						result.Execute = &R3000::LBU;
						break;
					}
					case 0x25: {
						result.Execute = &R3000::LHU;
						break;
					}
					case 0x26: {
						result.Execute = &R3000::LWR;
						break;
					}
					case 0x28: {
						result.Execute = &R3000::SB;
						break;
					}
					case 0x29: {
						result.Execute = &R3000::SH;
						break;
					}
					case 0x2A: {
						result.Execute = &R3000::SWL;
						break;
					}
					case 0x2B: {
						result.Execute = &R3000::SW;
						break;
					}
					case 0x2E: {
						result.Execute = &R3000::SWR;
						break;
					}

					case 0x30:
					case 0x31: 
					case 0x32:
					case 0x33: {
						if (CO_N(instruction) != 0x02) {
							if (!suppressException) raiseException(ExceptionType::CoprocessorUnusable);
							break;
						}

						result.Execute = &R3000::LWC2;
						break;
					}
					

					case 0x38:
					case 0x39:
					case 0x3A:
					case 0x3B: {
						if (CO_N(instruction) != 0x02) {
							if (!suppressException) raiseException(ExceptionType::CoprocessorUnusable);
							break;
						}

						result.Execute = &R3000::SWC2;
						break;
					}

					default: {
						if (!suppressException) raiseException(ExceptionType::ReservedInstruction);
						break;
					}
				}
			}

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

	void R3000::COP2()
	{
		ESX_CORE_LOG_ERROR("GTE command {:08X}h Not implemented yet", mCurrentInstruction.Immediate25());
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

	void R3000::LWC2()
	{
		ESX_CORE_LOG_ERROR("GTE Not implemented yet");
	}

	void R3000::SWC2()
	{
		ESX_CORE_LOG_ERROR("GTE Not implemented yet");
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