#include "R3000.h"

#include <iostream>
#include <iomanip>
#include <fstream>

namespace esx {

	R3000::R3000()
		: BusDevice(ESX_TEXT("R3000"))
	{
		mPC = 0x1FC00000;
		mNextPC = 0x1FC00000 + 4;

		mICache.resize(KIBI(4));
	}

	R3000::~R3000()
	{
	}

	void R3000::clock()
	{
		U32 opcode = fetch(mPC);

		Instruction instruction = decode(opcode, mPC);

		ESX_CORE_ASSERT(instruction.Execute, "No Operation");

		mPC = mNextPC;
		mNextPC += 4;

		bool pendingLoadsEmpty = mPendingLoads.empty();

		instruction.Execute(instruction);

		if (!pendingLoadsEmpty) {
			auto pendingLoad = mPendingLoads.front();
			mPendingLoads.pop();

			setRegister(pendingLoad.first, pendingLoad.second);
		}
	}


	U32 R3000::fetch(U32 address)
	{
		return load<U32>(address);
	}

	Instruction R3000::decode(U32 instruction, U32 address)
	{
		constexpr static StringView registersMnemonics[] = {
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

		Instruction result;

		result.Address = address;
		result.Opcode = OPCODE(instruction);
		result.Execute = nullptr;

		switch (result.Opcode) {
			//R Type
			case 0x00: {
				result.RegisterSource = RS(instruction);
				result.RegisterTarget = RT(instruction);
				result.RegisterDestination = RD(instruction);
				result.ShiftAmount = SHAMT(instruction);
				result.Function = FUNCT(instruction);


				switch (result.Function) {
					case 0x00: {
						result.Mnemonic = FormatString(ESX_TEXT("sll {},{},0x{:02x}"), registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterTarget], result.ShiftAmount);
						result.Execute = std::bind(&R3000::SLL, this, std::placeholders::_1);
						break;
					}
					case 0x02: {
						result.Mnemonic = FormatString(ESX_TEXT("srl {},{},0x{:02x}"), registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterTarget], result.ShiftAmount);
						result.Execute = std::bind(&R3000::SRL, this, std::placeholders::_1);
						break;
					}
					case 0x03: {
						result.Mnemonic = FormatString(ESX_TEXT("sra {},{},0x{:02x}"), registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterTarget], result.ShiftAmount);
						result.Execute = std::bind(&R3000::SRA, this, std::placeholders::_1);
						break;
					}
					case 0x04: {
						result.Mnemonic = FormatString(ESX_TEXT("sllv {},{},{}"), registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterTarget], registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::SLLV, this, std::placeholders::_1);
						break;
					}
					case 0x06: {
						result.Mnemonic = FormatString(ESX_TEXT("srlv {},{},{}"), registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterTarget], registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::SRLV, this, std::placeholders::_1);
						break;
					}
					case 0x07: {
						result.Mnemonic = FormatString(ESX_TEXT("srav {},{},{}"), registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterTarget], registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::SRAV, this, std::placeholders::_1);
						break;
					}
					case 0x08: {
						result.Mnemonic = FormatString(ESX_TEXT("jr {}"), registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::JR, this, std::placeholders::_1);
						break;
					}
					case 0x09: {
						result.Mnemonic = FormatString(ESX_TEXT("jalr {},{}"), registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::JALR, this, std::placeholders::_1);
						break;
					}
					case 0x0C: {
						result.Code = CODE(instruction);
						result.Mnemonic = FormatString(ESX_TEXT("syscall"));
						result.Execute = std::bind(&R3000::SYSCALL, this, std::placeholders::_1);
						break;
					}
					case 0x0D: {
						result.Code = CODE(instruction);
						result.Mnemonic = FormatString(ESX_TEXT("break"));
						result.Execute = std::bind(&R3000::BREAK, this, std::placeholders::_1);
						break;
					}
					case 0x10: {
						result.Mnemonic = FormatString(ESX_TEXT("mfhi {}"), registersMnemonics[result.RegisterDestination]);
						result.Execute = std::bind(&R3000::MFHI, this, std::placeholders::_1);
						break;
					}
					case 0x11: {
						result.Mnemonic = FormatString(ESX_TEXT("mthi {}"), registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::MTHI, this, std::placeholders::_1);
						break;
					}
					case 0x12: {
						result.Mnemonic = FormatString(ESX_TEXT("mflo {}"), registersMnemonics[result.RegisterDestination]);
						result.Execute = std::bind(&R3000::MFLO, this, std::placeholders::_1);
						break;
					}
					case 0x13: {
						result.Mnemonic = FormatString(ESX_TEXT("mtlo {}"), registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::MTLO, this, std::placeholders::_1);
						break;
					}
					case 0x18: {
						result.Mnemonic = FormatString(ESX_TEXT("mult {},{}"), registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::MULT, this, std::placeholders::_1);
						break;
					}
					case 0x19: {
						result.Mnemonic = FormatString(ESX_TEXT("multu {},{}"), registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::MULTU, this, std::placeholders::_1);
						break;
					}
					case 0x1A: {
						result.Mnemonic = FormatString(ESX_TEXT("div {},{}"), registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::DIV, this, std::placeholders::_1);
						break;
					}
					case 0x1B:{
						result.Mnemonic = FormatString(ESX_TEXT("divu {},{}"), registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::DIVU, this, std::placeholders::_1);
						break;
					}
					case 0x20: {
						if (result.RegisterTarget == 0) {
							result.Mnemonic = FormatString(ESX_TEXT("move {},{}"), registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource]);
						} else {
							result.Mnemonic = FormatString(ESX_TEXT("add {},{},{}"), registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						}
						result.Execute = std::bind(&R3000::ADD, this, std::placeholders::_1);
						break;
					}
					case 0x21: {
						if (result.RegisterTarget == 0) {
							result.Mnemonic = FormatString(ESX_TEXT("move {},{}"), registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource]);
						} else {
							result.Mnemonic = FormatString(ESX_TEXT("addu {},{},{}"), registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						}
						result.Execute = std::bind(&R3000::ADDU, this, std::placeholders::_1);
						break;
					}
					case 0x22: {
						result.Mnemonic = FormatString(ESX_TEXT("sub {},{},{}"), registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::SUB, this, std::placeholders::_1);
						break;
					}
					case 0x23: {
						result.Mnemonic = FormatString(ESX_TEXT("subu {},{},{}"), registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::SUBU, this, std::placeholders::_1);
						break;
					}
					case 0x24: {
						result.Mnemonic = FormatString(ESX_TEXT("and {},{},{}"), registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::AND, this, std::placeholders::_1);
						break;
					}
					case 0x25: {
						result.Mnemonic = FormatString(ESX_TEXT("or {},{},{}"), registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::OR, this, std::placeholders::_1);
						break;
					}
					case 0x26: {
						result.Mnemonic = FormatString(ESX_TEXT("xor {},{},{}"), registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::XOR, this, std::placeholders::_1);
						break;
					}
					case 0x27: {
						result.Mnemonic = FormatString(ESX_TEXT("nor {},{},{}"), registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::NOR, this, std::placeholders::_1);
						break;
					}
					case 0x2A: {
						result.Mnemonic = FormatString(ESX_TEXT("slt {},{},{}"), registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::SLT, this, std::placeholders::_1);
						break;
					}
					case 0x2B: {
						result.Mnemonic = FormatString(ESX_TEXT("sltu {},{},{}"), registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::SLTU, this, std::placeholders::_1);
						break;
					}
					default: {
						raiseException(ExceptionType::ReservedInstruction);
						break;
					}
				}

				break;
			}

			//J Type
			case 0x02: {
				result.PseudoAddress = ADDRESS(instruction);
				result.Mnemonic = FormatString(ESX_TEXT("j 0x{:08x}"), ((address + 4) & 0xF0000000) | (result.PseudoAddress << 2));
				result.Execute = std::bind(&R3000::J, this, std::placeholders::_1);
				break;
			}
			case 0x03: {
				result.PseudoAddress = ADDRESS(instruction);
				result.Mnemonic = FormatString(ESX_TEXT("jal 0x{:08x}"), ((address + 4) & 0xF0000000) | (result.PseudoAddress << 2));
				result.Execute = std::bind(&R3000::JAL, this, std::placeholders::_1);
				break;
			}

			default: {
				result.RegisterSource = RS(instruction);
				result.RegisterTarget = RT(instruction);
				result.Immediate = IMM(instruction);

				switch (result.Opcode) {
					case 0x01: {
						switch (result.RegisterTarget) {
							case 0x00: {
								result.Mnemonic = FormatString(ESX_TEXT("bltz {},0x{:08x}"), registersMnemonics[result.RegisterSource], (address + 4) + (SIGNEXT16(result.Immediate) << 2));
								result.Execute = std::bind(&R3000::BLTZ, this, std::placeholders::_1);
								break;
							}
							case 0x01: {
								result.Mnemonic = FormatString(ESX_TEXT("bgez {},0x{:08x}"), registersMnemonics[result.RegisterSource], (address + 4) + (SIGNEXT16(result.Immediate) << 2));
								result.Execute = std::bind(&R3000::BGEZ, this, std::placeholders::_1);
								break;
							}
							case 0x10: {
								result.Mnemonic = FormatString(ESX_TEXT("bltzal {},0x{:08x}"), registersMnemonics[result.RegisterSource], (address + 4) + (SIGNEXT16(result.Immediate) << 2));
								result.Execute = std::bind(&R3000::BLTZAL, this, std::placeholders::_1);
								break;
							}
							case 0x11: {
								result.Mnemonic = FormatString(ESX_TEXT("bgezal {},0x{:08x}"), registersMnemonics[result.RegisterSource], (address + 4) + (SIGNEXT16(result.Immediate) << 2));
								result.Execute = std::bind(&R3000::BGEZAL, this, std::placeholders::_1);
								break;
							}

							default: {
								raiseException(ExceptionType::ReservedInstruction);
								break;
							}
						}
						
						break;
					}
					case 0x04: {
						result.Mnemonic = FormatString(ESX_TEXT("beq {},{},0x{:08x}"), registersMnemonics[result.RegisterTarget], registersMnemonics[result.RegisterSource], (address + 4) + (SIGNEXT16(result.Immediate) << 2));
						result.Execute = std::bind(&R3000::BEQ, this, std::placeholders::_1);
						break;
					}
					case 0x05: {
						result.Mnemonic = FormatString(ESX_TEXT("bne {},{},0x{:08x}"), registersMnemonics[result.RegisterTarget], registersMnemonics[result.RegisterSource], (address + 4) + (SIGNEXT16(result.Immediate) << 2));
						result.Execute = std::bind(&R3000::BNE, this, std::placeholders::_1);
						break;
					}
					case 0x06: {
						result.Mnemonic = FormatString(ESX_TEXT("blez {},0x{:08x}"), registersMnemonics[result.RegisterSource], (address + 4) + (SIGNEXT16(result.Immediate) << 2));
						result.Execute = std::bind(&R3000::BLEZ, this, std::placeholders::_1);
						break;
					}
					case 0x07: {
						result.Mnemonic = FormatString(ESX_TEXT("bgtz {},0x{:08x}"), registersMnemonics[result.RegisterSource], (address + 4) + (SIGNEXT16(result.Immediate) << 2));
						result.Execute = std::bind(&R3000::BGTZ, this, std::placeholders::_1);
						break;
					}
					case 0x08: {
						result.Mnemonic = FormatString(ESX_TEXT("addi {},{},0x{:04x}"), registersMnemonics[result.RegisterTarget], registersMnemonics[result.RegisterSource], (I16)result.Immediate);
						result.Execute = std::bind(&R3000::ADDI, this, std::placeholders::_1);
						break;
					}
					case 0x09: {
						result.Mnemonic = FormatString(ESX_TEXT("addiu {},{},0x{:04x}"), registersMnemonics[result.RegisterTarget], registersMnemonics[result.RegisterSource], (I16)result.Immediate);
						result.Execute = std::bind(&R3000::ADDIU, this, std::placeholders::_1);
						break;
					}
					case 0x0A: {
						result.Mnemonic = FormatString(ESX_TEXT("slti {},{},0x{:04x}"), registersMnemonics[result.RegisterTarget], registersMnemonics[result.RegisterSource], (I16)result.Immediate);
						result.Execute = std::bind(&R3000::SLTI, this, std::placeholders::_1);
						break;
					}
					case 0x0B: {
						result.Mnemonic = FormatString(ESX_TEXT("sltiu {},{},0x{:04x}"), registersMnemonics[result.RegisterTarget], registersMnemonics[result.RegisterSource], (I16)result.Immediate);
						result.Execute = std::bind(&R3000::SLTIU, this, std::placeholders::_1);
						break;
					}
					case 0x0C: {
						result.Mnemonic = FormatString(ESX_TEXT("andi {},{},0x{:04x}"), registersMnemonics[result.RegisterTarget], registersMnemonics[result.RegisterSource], result.Immediate);
						result.Execute = std::bind(&R3000::ANDI, this, std::placeholders::_1);
						break;
					}
					case 0x0D: {
						result.Mnemonic = FormatString(ESX_TEXT("ori {},{},0x{:04x}"), registersMnemonics[result.RegisterTarget], registersMnemonics[result.RegisterSource], result.Immediate);
						result.Execute = std::bind(&R3000::ORI, this, std::placeholders::_1);
						break;
					}
					case 0x0E: {
						result.Mnemonic = FormatString(ESX_TEXT("xori {},{},0x{:04x}"), registersMnemonics[result.RegisterTarget], registersMnemonics[result.RegisterSource], result.Immediate);
						result.Execute = std::bind(&R3000::XORI, this, std::placeholders::_1);
						break;
					}
					case 0x0F: {
						result.Mnemonic = FormatString(ESX_TEXT("lui {},0x{:04x}"), registersMnemonics[result.RegisterTarget], result.Immediate);
						result.Execute = std::bind(&R3000::LUI, this, std::placeholders::_1);
						break;
					}
					case 0x10:
					case 0x11:
					case 0x12:
					case 0x13: {
						U8 cpn = CO_N(instruction);
						if (cpn != 0x00) {
							raiseException(ExceptionType::CoprocessorUnusable);
							break;
						}

						result.RegisterDestination = RD(instruction);

						if (CO(instruction) == 0) {
							switch (result.RegisterSource) {
								case 0x00: {
									result.Mnemonic = FormatString(ESX_TEXT("mfc{} {},${}"), cpn, registersMnemonics[result.RegisterTarget], result.RegisterDestination);
									result.Execute = std::bind(&R3000::MFC0, this, std::placeholders::_1);
									break;
								}
								case 0x04: {
									result.Mnemonic = FormatString(ESX_TEXT("mtc{} {},${}"), cpn, registersMnemonics[result.RegisterTarget], result.RegisterDestination);
									result.Execute = std::bind(&R3000::MTC0, this, std::placeholders::_1);
									break;
								}
								default: {
									raiseException(ExceptionType::ReservedInstruction);
									break;
								}
							}
						}
						else {
							switch (COP_FUNC(instruction)) {
								case 0x10: {
									result.Mnemonic = FormatString(ESX_TEXT("rfe"));
									result.Execute = std::bind(&R3000::RFE, this, std::placeholders::_1);
									break;
								}
								
								default: {
									raiseException(ExceptionType::ReservedInstruction);
									break;
								}
							}
						}
						
						break;
					}
					case 0x20: {
						result.Mnemonic = FormatString(ESX_TEXT("lb {},0x{:04x}({})"), registersMnemonics[result.RegisterTarget], result.Immediate, registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::LB, this, std::placeholders::_1);
						break;
					}
					case 0x21: {
						result.Mnemonic = FormatString(ESX_TEXT("lh {},0x{:04x}({})"), registersMnemonics[result.RegisterTarget], result.Immediate, registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::LH, this, std::placeholders::_1);
						break;
					}
					case 0x22: {
						result.Mnemonic = FormatString(ESX_TEXT("lwl {},0x{:04x}({})"), registersMnemonics[result.RegisterTarget], result.Immediate, registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::LWL, this, std::placeholders::_1);
						break;
					}
					case 0x23: {
						result.Mnemonic = FormatString(ESX_TEXT("lw {},0x{:04x}({})"), registersMnemonics[result.RegisterTarget], result.Immediate, registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::LW, this, std::placeholders::_1);
						break;
					}
					case 0x24: {
						result.Mnemonic = FormatString(ESX_TEXT("lbu {},0x{:04x}({})"), registersMnemonics[result.RegisterTarget], result.Immediate, registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::LBU, this, std::placeholders::_1);
						break;
					}
					case 0x25: {
						result.Mnemonic = FormatString(ESX_TEXT("lhu {},0x{:04x}({})"), registersMnemonics[result.RegisterTarget], result.Immediate, registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::LHU, this, std::placeholders::_1);
						break;
					}
					case 0x26: {
						result.Mnemonic = FormatString(ESX_TEXT("lwr {},0x{:04x}({})"), registersMnemonics[result.RegisterTarget], result.Immediate, registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::LWR, this, std::placeholders::_1);
						break;
					}
					case 0x28: {
						result.Mnemonic = FormatString(ESX_TEXT("sb {},0x{:04x}({})"), registersMnemonics[result.RegisterTarget], result.Immediate, registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::SB, this, std::placeholders::_1);
						break;
					}
					case 0x29: {
						result.Mnemonic = FormatString(ESX_TEXT("sh {},0x{:04x}({})"), registersMnemonics[result.RegisterTarget], result.Immediate, registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::SH, this, std::placeholders::_1);
						break;
					}
					case 0x2A: {
						result.Mnemonic = FormatString(ESX_TEXT("swl {},0x{:04x}({})"), registersMnemonics[result.RegisterTarget], result.Immediate, registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::SWL, this, std::placeholders::_1);
						break;
					}
					case 0x2B: {
						result.Mnemonic = FormatString(ESX_TEXT("sw {},0x{:04x}({})"), registersMnemonics[result.RegisterTarget], result.Immediate, registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::SW, this, std::placeholders::_1);
						break;
					}
					case 0x2E: {
						result.Mnemonic = FormatString(ESX_TEXT("swr {},0x{:04x}({})"), registersMnemonics[result.RegisterTarget], result.Immediate, registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::SWR, this, std::placeholders::_1);
						break;
					}

					case 0x30:
					case 0x31:
					case 0x32:
					case 0x33: {
						//LWCx
						raiseException(ExceptionType::CoprocessorUnusable);
						break;
					}

					case 0x38:
					case 0x39:
					case 0x3A:
					case 0x3B: {
						//LWCx
						raiseException(ExceptionType::CoprocessorUnusable);
						break;
					}

					default: {
						raiseException(ExceptionType::ReservedInstruction);
						break;
					}
				}
			}

		}

		return result;
	}

	void R3000::ADD(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = getRegister(instruction.RegisterTarget);

		U32 r = a + b;

		if (OVERFLOW_ADD32(a, b, r)) {
			raiseException(ExceptionType::ArithmeticOverflow, &instruction);
			return;
		}

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::ADDU(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = getRegister(instruction.RegisterTarget);

		U32 r = a + b;

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::SUB(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = getRegister(instruction.RegisterTarget);

		U32 r = a - b;

		if (OVERFLOW_SUB32(a, b, r)) {
			raiseException(ExceptionType::ArithmeticOverflow, &instruction);
			return;
		}

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::SUBU(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = getRegister(instruction.RegisterTarget);

		U32 r = a - b;

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::ADDI(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = SIGNEXT16(instruction.Immediate);

		U32 r = a + b;

		if (OVERFLOW_ADD32(a, b, r)) {
			raiseException(ExceptionType::ArithmeticOverflow, &instruction);
			return;
		}

		setRegister(instruction.RegisterTarget, r);
	}

	void R3000::ADDIU(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = SIGNEXT16(instruction.Immediate);

		U32 r = a + b;

		setRegister(instruction.RegisterTarget, r);
	}

	void R3000::MULT(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = getRegister(instruction.RegisterTarget);

		U64 r = a * b;

		mHI = (r >> 32) & 0xFFFFFFFF;
		mLO = r & 0xFFFFFFFF;
	}

	void R3000::MULTU(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = getRegister(instruction.RegisterTarget);

		U64 r = a * b;

		mHI = (r >> 32) & 0xFFFFFFFF;
		mLO = r & 0xFFFFFFFF;
	}

	void R3000::DIV(const Instruction& instruction)
	{
		I32 a = getRegister(instruction.RegisterSource);
		I32 b = getRegister(instruction.RegisterTarget);

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

	void R3000::DIVU(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = getRegister(instruction.RegisterTarget);

		if (b != 0) {
			mHI = a % b;
			mLO = a / b;
		} else {
			mHI = a;
			mLO = 0xFFFFFFFF;
		}
	}

	void R3000::MFLO(const Instruction& instruction)
	{
		setRegister(instruction.RegisterDestination, mLO);
	}

	void R3000::MTLO(const Instruction& instruction)
	{
		mLO = getRegister(instruction.RegisterSource);
	}

	void R3000::MFHI(const Instruction& instruction)
	{
		setRegister(instruction.RegisterDestination, mHI);
	}

	void R3000::MTHI(const Instruction& instruction)
	{
		mHI = getRegister(instruction.RegisterSource);
	}

	void R3000::LW(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = SIGNEXT16(instruction.Immediate);

		U32 m = a + b;

		U32 r = load<U32>(m);

		addPendingLoad(instruction.RegisterTarget, r);
	}

	void R3000::LH(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = SIGNEXT16(instruction.Immediate);

		U32 m = a + b;

		U32 r = load<U16>(m);
		r = SIGNEXT16(r);

		addPendingLoad(instruction.RegisterTarget, r);
	}

	void R3000::LHU(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = SIGNEXT16(instruction.Immediate);

		U32 m = a + b;

		U32 r = load<U16>(m);

		addPendingLoad(instruction.RegisterTarget, r);
	}

	void R3000::LB(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = SIGNEXT16(instruction.Immediate);

		U32 m = a + b;

		U32 r = load<U8>(m);
		r = SIGNEXT8(r);

		addPendingLoad(instruction.RegisterTarget, r);
	}

	void R3000::LBU(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = SIGNEXT16(instruction.Immediate);

		U32 m = a + b;

		U32 r = load<U8>(m);

		addPendingLoad(instruction.RegisterTarget, r);
	}

	void R3000::LWL(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = SIGNEXT16(instruction.Immediate);
		U32 c = getRegister(instruction.RegisterTarget);

		U32 m = a + b;

		U32 am = m & ~(0x3);
		U32 aw = load<U32>(am);

		U32 u = m & (0x3);
		U32 r = (c & (0x00FFFFFF >> (u * 8))) | (aw << (24 - (u * 8)));

		setRegister(instruction.RegisterTarget, r);
	}

	void R3000::LWR(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = SIGNEXT16(instruction.Immediate);
		U32 c = getRegister(instruction.RegisterTarget);

		U32 m = a + b;

		U32 am = m & ~(0x3);
		U32 aw = load<U32>(am);

		U32 u = m & (0x3);
		U32 r = (c & (0xFFFFFF00 << ((0x3 - u) * 8))) | (aw >> (u * 8));

		setRegister(instruction.RegisterTarget, r);
	}

	void R3000::SWL(const Instruction& instruction)
	{
		ESX_CORE_ASSERT(false, "SWL not implemented yet");
	}

	void R3000::SWR(const Instruction& instruction)
	{
		ESX_CORE_ASSERT(false, "SWR not implemented yet");
	}

	void R3000::SB(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = SIGNEXT16(instruction.Immediate);
		U32 v = getRegister(instruction.RegisterTarget);

		U32 m = a + b;
		
		store<U8>(m, v);
	}

	void R3000::SH(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = SIGNEXT16(instruction.Immediate);
		U32 v = getRegister(instruction.RegisterTarget);

		U32 m = a + b;

		store<U16>(m, v);
	}

	void R3000::SW(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = SIGNEXT16(instruction.Immediate);

		U32 m = a + b;

		U32 v = getRegister(instruction.RegisterTarget);

		store<U32>(m, v);
	}

	void R3000::LUI(const Instruction& instruction)
	{
		U32 r = instruction.Immediate << 16;
		setRegister(instruction.RegisterTarget, r);
	}

	void R3000::SLT(const Instruction& instruction)
	{
		I32 a = getRegister(instruction.RegisterSource);
		I32 b = getRegister(instruction.RegisterTarget);

		U32 r = a < b;

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::SLTU(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = getRegister(instruction.RegisterTarget);

		U32 r = a < b;

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::SLTI(const Instruction& instruction)
	{
		I32 a = getRegister(instruction.RegisterSource);
		I32 b = SIGNEXT16(instruction.Immediate);

		U32 r = a < b;

		setRegister(instruction.RegisterTarget, r);
	}

	void R3000::SLTIU(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = SIGNEXT16(instruction.Immediate);

		U32 r = a < b;

		setRegister(instruction.RegisterTarget, r);
	}

	void R3000::AND(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = getRegister(instruction.RegisterTarget);

		U32 r = a & b;

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::ANDI(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = instruction.Immediate;

		U32 r = a & b;

		setRegister(instruction.RegisterTarget, r);
	}

	void R3000::OR(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = getRegister(instruction.RegisterTarget);

		U32 r = a | b;

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::ORI(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = instruction.Immediate;

		U32 r = a | b;

		setRegister(instruction.RegisterTarget, r);
	}

	void R3000::XOR(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = getRegister(instruction.RegisterTarget);

		U32 r = a ^ b;

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::XORI(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = instruction.Immediate;

		U32 r = a ^ b;

		setRegister(instruction.RegisterTarget, r);
	}

	void R3000::NOR(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = getRegister(instruction.RegisterTarget);

		U32 r = ~(a | b);

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::SLL(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterTarget);
		U32 s = instruction.ShiftAmount;

		U32 r = a << s;

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::SRL(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterTarget);
		U32 s = instruction.ShiftAmount;

		U32 r = a >> s;

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::SRA(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterTarget);
		U32 s = instruction.ShiftAmount;

		U32 r = (a & 0x80000000 ? 0xFFFFFFFF << (32 - s) : 0x0) | (a >> s);

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::SLLV(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterTarget);
		U32 s = getRegister(instruction.RegisterSource);

		U32 r = a << s;

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::SRLV(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterTarget);
		U32 s = getRegister(instruction.RegisterSource);

		U32 r = a >> s;

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::SRAV(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterTarget);
		U32 s = getRegister(instruction.RegisterSource);

		U32 r = (a & 0x80000000 ? 0xFFFFFFFF << (32 - s) : 0x0) | (a >> s);

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::BEQ(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = getRegister(instruction.RegisterTarget);
		I32 o = SIGNEXT16(instruction.Immediate) << 2;

		if (a == b) {
			mNextPC += o;
			mNextPC -= 4;
		}
	}

	void R3000::BNE(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		U32 b = getRegister(instruction.RegisterTarget);
		I32 o = SIGNEXT16(instruction.Immediate) << 2;

		if (a != b) {
			mNextPC += o;
			mNextPC -= 4;
		}
	}

	void R3000::BLTZ(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		I32 o = SIGNEXT16(instruction.Immediate) << 2;

		if (a < 0) {
			mNextPC += o;
			mNextPC -= 4;
		}
	}

	void R3000::BLTZAL(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		I32 o = SIGNEXT16(instruction.Immediate) << 2;

		if (a < 0) {
			setRegister(31, mNextPC);
			mNextPC += o;
			mNextPC -= 4;
		}
	}

	void R3000::BLEZ(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		I32 o = SIGNEXT16(instruction.Immediate) << 2;

		if (a <= 0) {
			mNextPC += o;
			mNextPC -= 4;
		}
	}

	void R3000::BGTZ(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		I32 o = SIGNEXT16(instruction.Immediate) << 2;

		if (a > 0) {
			mNextPC += o;
			mNextPC -= 4;
		}
	}

	void R3000::BGEZ(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		I32 o = SIGNEXT16(instruction.Immediate) << 2;

		if (a >= 0) {
			mNextPC += o;
			mNextPC -= 4;
		}
	}

	void R3000::BGEZAL(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		I32 o = SIGNEXT16(instruction.Immediate) << 2;

		if (a >= 0) {
			setRegister(31, mNextPC);
			mNextPC += o;
			mNextPC -= 4;
		}
	}

	void R3000::J(const Instruction& instruction)
	{
		U32 a = (mNextPC & 0xF0000000) | (instruction.PseudoAddress << 2);
		mNextPC = a;
	}

	void R3000::JR(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		mNextPC = a;
	}

	void R3000::JAL(const Instruction& instruction)
	{
		U32 a = (mNextPC & 0xF0000000) | (instruction.PseudoAddress << 2);
		setRegister(31, mNextPC);
		mNextPC = a;
	}

	void R3000::JALR(const Instruction& instruction)
	{
		U32 a = getRegister(instruction.RegisterSource);
		setRegister(31, mNextPC);
		mNextPC = a;
	}

	void R3000::BREAK(const Instruction& instruction)
	{
		raiseException(ExceptionType::Breakpoint, &instruction);
	}

	void R3000::SYSCALL(const Instruction& instruction)
	{
		raiseException(ExceptionType::Syscall, &instruction);
	}

	void R3000::MTC0(const Instruction& instruction)
	{
		U32 sr = getCP0Register((U8)COP0Register::SR);
		U32 r = getRegister(instruction.RegisterTarget);

		if ((instruction.RegisterDestination >= 0 && instruction.RegisterDestination <= 2) ||
			instruction.RegisterDestination == 4 ||
			instruction.RegisterDestination == 10 ||
			(instruction.RegisterDestination >= 32 && instruction.RegisterDestination <= 63)) {
			raiseException(ExceptionType::ReservedInstruction, &instruction);
			return;
		}

		if (instruction.RegisterDestination < 16 && (sr & 0x10000002) == 0x1) {
			raiseException(ExceptionType::CoprocessorUnusable, &instruction);
			return;
		}

		switch ((COP0Register)instruction.RegisterDestination) {
			case COP0Register::Cause: {
				U32 t = getCP0Register(instruction.RegisterDestination);

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

		setCP0Register(instruction.RegisterDestination, r);
	}

	void R3000::MFC0(const Instruction& instruction)
	{
		U32 sr = getCP0Register((U8)COP0Register::SR);
		U32 r = getCP0Register(instruction.RegisterDestination);

		if (instruction.RegisterDestination < 16 && (sr & 0x10000002) == 0x1) {
			raiseException(ExceptionType::CoprocessorUnusable, &instruction);
			return;
		}

		setRegister(instruction.RegisterTarget, r);
	}

	void R3000::RFE(const Instruction& instruction)
	{
		U32 sr = getCP0Register((U8)COP0Register::SR);

		if ((sr & 0x10000002) == 0x1) {
			raiseException(ExceptionType::CoprocessorUnusable, &instruction);
			return;
		}

		U32 r = sr;

		r &= ~0xF; //Clear all first 4 bits
		r |= (sr >> 2) & 0xF;

		setCP0Register((U8)COP0Register::SR, r);
	}

	void R3000::addPendingLoad(U8 index, U32 value)
	{
		mPendingLoads.emplace(index, value);
	}

	void R3000::setRegister(U8 index, U32 value)
	{
		mRegisters[index] = value;
		mRegisters[0] = 0;
	}

	U32 R3000::getRegister(U8 index)
	{
		return mRegisters[index];
	}

	void R3000::setCP0Register(U8 index, U32 value)
	{
		mCP0Registers[index] = value;
	}

	U32 R3000::getCP0Register(U8 index)
	{
		return mCP0Registers[index];
	}

	void R3000::raiseException(ExceptionType type, const Instruction* instruction)
	{
		/*switch (type)
		{
		case ExceptionType::Interrupt:
			ESX_CORE_ASSERT(false,"ExceptionType::Interrupt not handled yet");
			break;

		case ExceptionType::AddressErrorLoad:
			ESX_CORE_ASSERT(false, "ExceptionType::AddressErrorLoad not handled yet");
			break;

		case ExceptionType::AddressErrorStore:
			ESX_CORE_ASSERT(false, "ExceptionType::AddressErrorStore not handled yet");
			break;

		case ExceptionType::Syscall:
			ESX_CORE_ASSERT(false, "ExceptionType::Syscall not handled yet");
			break;

		case ExceptionType::Breakpoint:
			ESX_CORE_ASSERT(false, "ExceptionType::Breakpoint not handled yet");
			break;

		case ExceptionType::ReservedInstruction:
			ESX_CORE_ASSERT(false, "ExceptionType::ReservedInstruction not handled yet");
			break;

		case ExceptionType::CoprocessorUnusable:
			ESX_CORE_ASSERT(false, "ExceptionType::CoprocessorUnusable not handled yet");
			break;

		case ExceptionType::ArithmeticOverflow:
			ESX_CORE_ASSERT(false, "ExceptionType::ArithmeticOverflow not handled yet");
			break;
		
		}*/
	}

}