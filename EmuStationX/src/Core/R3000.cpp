#include "R3000.h"

#include <iostream>
#include <iomanip>
#include <fstream>

namespace esx {

	static std::ofstream of;

	R3000::R3000()
		: BusDevice("R3000")
	{
		mPC = 0xBFC00000;

		mNextInstruction = decode(0x0);

		of.open("instructions.log", std::ofstream::binary);
	}

	R3000::~R3000()
	{
	}

	void R3000::clock()
	{
		assert(mNextInstruction.Execute && "No Operation");

		Instruction instruction = mNextInstruction;

		uint32_t opcode = fetch(mPC);

		mNextInstruction = decode(opcode);

		mPC += 4;

		of << "0x" << std::setw(8) << std::setfill('0') << std::hex << instruction.Address << " : " << instruction.Mnemonic << std::endl;
		instruction.Execute(instruction);
	}


	uint32_t R3000::fetch(uint32_t address)
	{
		return load<uint32_t>(address);
	}

	Instruction R3000::decode(uint32_t instruction)
	{
		constexpr static std::string_view registersMnemonics[] = {
			"$zero",
			"$at",
			"$v0","$v1",
			"$a0","$a1","$a2","$a3",
			"$t0","$t1","$t2","$t3","$t4","$t5","$t6","$t7",
			"$s0","$s1","$s2","$s3","$s4","$s5","$s6","$s7",
			"$t8","$t9",
			"$k0","$k1",
			"$gp",
			"$sp",
			"$fp",
			"$ra"
		};

		Instruction result;

		result.Address = mPC;
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
						result.Mnemonic = std::format("sll {},{},{}", registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::SLL, this, std::placeholders::_1);
						break;
					}
					case 0x02: {
						result.Mnemonic = std::format("srl {},{},{}", registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::SRL, this, std::placeholders::_1);
						break;
					}
					case 0x03: {
						result.Mnemonic = std::format("sra {},{},{}", registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::SRA, this, std::placeholders::_1);
						break;
					}
					case 0x04: {
						result.Mnemonic = std::format("sllv {},{},{}", registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::SLLV, this, std::placeholders::_1);
						break;
					}
					case 0x06: {
						result.Mnemonic = std::format("srlv {},{},{}", registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::SRLV, this, std::placeholders::_1);
						break;
					}
					case 0x07: {
						result.Mnemonic = std::format("srav {},{},{}", registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::SRAV, this, std::placeholders::_1);
						break;
					}
					case 0x08: {
						result.Mnemonic = std::format("jr {}", registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::JR, this, std::placeholders::_1);
						break;
					}
					case 0x09: {
						result.Mnemonic = std::format("jalr {},{}", registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::JALR, this, std::placeholders::_1);
						break;
					}
					case 0x10: {
						result.Mnemonic = std::format("mfhi {}", registersMnemonics[result.RegisterDestination]);
						result.Execute = std::bind(&R3000::MFHI, this, std::placeholders::_1);
						break;
					}
					case 0x11: {
						result.Mnemonic = std::format("mthi {}", registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::MTHI, this, std::placeholders::_1);
						break;
					}
					case 0x12: {
						result.Mnemonic = std::format("mflo {}", registersMnemonics[result.RegisterDestination]);
						result.Execute = std::bind(&R3000::MFLO, this, std::placeholders::_1);
						break;
					}
					case 0x13: {
						result.Mnemonic = std::format("mtlo {}", registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::MTLO, this, std::placeholders::_1);
						break;
					}
					case 0x18: {
						result.Mnemonic = std::format("mult {},{}", registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::MULT, this, std::placeholders::_1);
						break;
					}
					case 0x19: {
						result.Mnemonic = std::format("multu {},{}", registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::MULTU, this, std::placeholders::_1);
						break;
					}
					case 0x1A: {
						result.Mnemonic = std::format("div {},{}", registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::DIV, this, std::placeholders::_1);
						break;
					}
					case 0x1B:{
						result.Mnemonic = std::format("divu {},{}", registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::DIVU, this, std::placeholders::_1);
						break;
					}
					case 0x20: {
						result.Mnemonic = std::format("add {},{},{}", registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::ADD, this, std::placeholders::_1);
						break;
					}
					case 0x21: {
						result.Mnemonic = std::format("addu {},{},{}", registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::ADDU, this, std::placeholders::_1);
						break;
					}
					case 0x22: {
						result.Mnemonic = std::format("sub {},{},{}", registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::SUB, this, std::placeholders::_1);
						break;
					}
					case 0x23: {
						result.Mnemonic = std::format("subu {},{},{}", registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::SUBU, this, std::placeholders::_1);
						break;
					}
					case 0x24: {
						result.Mnemonic = std::format("and {},{},{}", registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::AND, this, std::placeholders::_1);
						break;
					}
					case 0x25: {
						result.Mnemonic = std::format("or {},{},{}", registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::OR, this, std::placeholders::_1);
						break;
					}
					case 0x26: {
						result.Mnemonic = std::format("xor {},{},{}", registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::XOR, this, std::placeholders::_1);
						break;
					}
					case 0x27: {
						result.Mnemonic = std::format("nor {},{},{}", registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::NOR, this, std::placeholders::_1);
						break;
					}
					case 0x2A: {
						result.Mnemonic = std::format("slt {},{},{}", registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::SLT, this, std::placeholders::_1);
						break;
					}
					case 0x2B: {
						result.Mnemonic = std::format("sltu {},{},{}", registersMnemonics[result.RegisterDestination], registersMnemonics[result.RegisterSource], registersMnemonics[result.RegisterTarget]);
						result.Execute = std::bind(&R3000::SLTU, this, std::placeholders::_1);
						break;
					}
				}

				break;
			}

			//J Type
			case 0x02: {
				result.PseudoAddress = ADDRESS(instruction);
				result.Mnemonic = std::format("j 0x{:x}", (mPC & 0xF0000000) | (result.PseudoAddress << 2));
				result.Execute = std::bind(&R3000::J, this, std::placeholders::_1);
				break;
			}
			case 0x03: {
				result.PseudoAddress = ADDRESS(instruction);
				result.Mnemonic = std::format("jal 0x{:x}", (mPC & 0xF0000000) | (result.PseudoAddress << 2));
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
								result.Mnemonic = std::format("bltz {},{}", registersMnemonics[result.RegisterSource], (int16_t)result.Immediate);
								result.Execute = std::bind(&R3000::BLTZ, this, std::placeholders::_1);
								break;
							}
							case 0x01: {
								result.Mnemonic = std::format("bgez {},{}", registersMnemonics[result.RegisterSource], (int16_t)result.Immediate);
								result.Execute = std::bind(&R3000::BGEZ, this, std::placeholders::_1);
								break;
							}
							case 0x10: {
								result.Mnemonic = std::format("bltzal {},{}", registersMnemonics[result.RegisterSource], (int16_t)result.Immediate);
								result.Execute = std::bind(&R3000::BLTZAL, this, std::placeholders::_1);
								break;
							}
							case 0x11: {
								result.Mnemonic = std::format("bgezal {},{}", registersMnemonics[result.RegisterSource], (int16_t)result.Immediate);
								result.Execute = std::bind(&R3000::BGEZAL, this, std::placeholders::_1);
								break;
							}
						}
						
						break;
					}
					case 0x04: {
						result.Mnemonic = std::format("beq {},{},{}", registersMnemonics[result.RegisterTarget], registersMnemonics[result.RegisterSource], (int16_t)result.Immediate);
						result.Execute = std::bind(&R3000::BEQ, this, std::placeholders::_1);
						break;
					}
					case 0x05: {
						result.Mnemonic = std::format("bne {},{},{}", registersMnemonics[result.RegisterTarget], registersMnemonics[result.RegisterSource], (int16_t)result.Immediate);
						result.Execute = std::bind(&R3000::BNE, this, std::placeholders::_1);
						break;
					}
					case 0x06: {
						result.Mnemonic = std::format("blez {},{}", registersMnemonics[result.RegisterSource], (int16_t)result.Immediate);
						result.Execute = std::bind(&R3000::BLEZ, this, std::placeholders::_1);
						break;
					}
					case 0x07: {
						result.Mnemonic = std::format("bgtz {},{}", registersMnemonics[result.RegisterSource], (int16_t)result.Immediate);
						result.Execute = std::bind(&R3000::BGTZ, this, std::placeholders::_1);
						break;
					}
					case 0x08: {
						result.Mnemonic = std::format("addi {},{},{}", registersMnemonics[result.RegisterTarget], registersMnemonics[result.RegisterSource], (int16_t)result.Immediate);
						result.Execute = std::bind(&R3000::ADDI, this, std::placeholders::_1);
						break;
					}
					case 0x09: {
						result.Mnemonic = std::format("addiu {},{},{}", registersMnemonics[result.RegisterTarget], registersMnemonics[result.RegisterSource], (int16_t)result.Immediate);
						result.Execute = std::bind(&R3000::ADDIU, this, std::placeholders::_1);
						break;
					}
					case 0x0A: {
						result.Mnemonic = std::format("slti {},{},{}", registersMnemonics[result.RegisterTarget], registersMnemonics[result.RegisterSource], (int16_t)result.Immediate);
						result.Execute = std::bind(&R3000::SLTI, this, std::placeholders::_1);
						break;
					}
					case 0x0B: {
						result.Mnemonic = std::format("sltiu {},{},{}", registersMnemonics[result.RegisterTarget], registersMnemonics[result.RegisterSource], (int16_t)result.Immediate);
						result.Execute = std::bind(&R3000::SLTIU, this, std::placeholders::_1);
						break;
					}
					case 0x0C: {
						result.Mnemonic = std::format("andi {},{},{}", registersMnemonics[result.RegisterTarget], registersMnemonics[result.RegisterSource], result.Immediate);
						result.Execute = std::bind(&R3000::ANDI, this, std::placeholders::_1);
						break;
					}
					case 0x0D: {
						result.Mnemonic = std::format("ori {},{},{}", registersMnemonics[result.RegisterTarget], registersMnemonics[result.RegisterSource], result.Immediate);
						result.Execute = std::bind(&R3000::ORI, this, std::placeholders::_1);
						break;
					}
					case 0x0E: {
						result.Mnemonic = std::format("xori {},{},{}", registersMnemonics[result.RegisterTarget], registersMnemonics[result.RegisterSource], result.Immediate);
						result.Execute = std::bind(&R3000::XORI, this, std::placeholders::_1);
						break;
					}
					case 0x0F: {
						result.Mnemonic = std::format("lui {},{}", registersMnemonics[result.RegisterTarget], result.Immediate);
						result.Execute = std::bind(&R3000::LUI, this, std::placeholders::_1);
						break;
					}
					case 0x10: {
						result.RegisterDestination = RD(instruction);

						switch (result.RegisterSource) {
							case 0x00: {
								result.Mnemonic = std::format("mfc0 {},${}", registersMnemonics[result.RegisterTarget], result.RegisterDestination);
								result.Execute = std::bind(&R3000::MFC0, this, std::placeholders::_1);
								break;
							}
							case 0x04: {
								result.Mnemonic = std::format("mtc0 {},${}", registersMnemonics[result.RegisterTarget], result.RegisterDestination);
								result.Execute = std::bind(&R3000::MTC0, this, std::placeholders::_1);
								break;
							}
							case 0x10: {
								result.Mnemonic = std::format("rfe");
								result.Execute = std::bind(&R3000::RFE, this, std::placeholders::_1);
								break;
							}
						}
						
						break;
					}
					case 0x11: {
						assert(false && "Coprocessor 1 not supported yet.");
						break;
					}
					case 0x12: {
						assert(false && "GTE not implemented yet.");
						break;
					}
					case 0x13: {
						assert(false && "Coprocessor 3 not supported yet.");

						break;
					}
					case 0x20: {
						result.Mnemonic = std::format("lb {},{}({})", registersMnemonics[result.RegisterTarget], result.Immediate, registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::LB, this, std::placeholders::_1);
						break;
					}
					case 0x21: {
						result.Mnemonic = std::format("lh {},{}({})", registersMnemonics[result.RegisterTarget], result.Immediate, registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::LH, this, std::placeholders::_1);
						break;
					}
					case 0x22: {
						result.Mnemonic = std::format("lwl {},{}({})", registersMnemonics[result.RegisterTarget], result.Immediate, registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::LWL, this, std::placeholders::_1);
						break;
					}
					case 0x23: {
						result.Mnemonic = std::format("lw {},{}({})", registersMnemonics[result.RegisterTarget], result.Immediate, registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::LW, this, std::placeholders::_1);
						break;
					}
					case 0x24: {
						result.Mnemonic = std::format("lbu {},{}({})", registersMnemonics[result.RegisterTarget], result.Immediate, registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::LBU, this, std::placeholders::_1);
						break;
					}
					case 0x25: {
						result.Mnemonic = std::format("lhu {},{}({})", registersMnemonics[result.RegisterTarget], result.Immediate, registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::LHU, this, std::placeholders::_1);
						break;
					}
					case 0x26: {
						result.Mnemonic = std::format("lwr {},{}({})", registersMnemonics[result.RegisterTarget], result.Immediate, registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::LWR, this, std::placeholders::_1);
						break;
					}
					case 0x28: {
						result.Mnemonic = std::format("sb {},{}({})", registersMnemonics[result.RegisterTarget], result.Immediate, registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::SB, this, std::placeholders::_1);
						break;
					}
					case 0x29: {
						result.Mnemonic = std::format("sh {},{}({})", registersMnemonics[result.RegisterTarget], result.Immediate, registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::SH, this, std::placeholders::_1);
						break;
					}
					case 0x2A: {
						result.Mnemonic = std::format("swl {},{}({})", registersMnemonics[result.RegisterTarget], result.Immediate, registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::SWL, this, std::placeholders::_1);
						break;
					}
					case 0x2B: {
						result.Mnemonic = std::format("sw {},{}({})", registersMnemonics[result.RegisterTarget], result.Immediate, registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::SW, this, std::placeholders::_1);
						break;
					}
					case 0x2E: {
						result.Mnemonic = std::format("swr {},{}({})", registersMnemonics[result.RegisterTarget], result.Immediate, registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::SWR, this, std::placeholders::_1);
						break;
					}
				}
			}

		}

		return result;
	}

	void R3000::ADD(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = getRegister(instruction.RegisterTarget);

		uint32_t r = a + b;

		if (OVERFLOW_ADD32(a, b, r)) {
			//TODO: Exception Overflow
		}

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::ADDU(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = getRegister(instruction.RegisterTarget);

		uint32_t r = a + b;

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::SUB(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = getRegister(instruction.RegisterTarget);

		uint32_t r = a - b;

		if (OVERFLOW_SUB32(a, b, r)) {
			//TODO: Exception Overflow
		}

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::SUBU(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = getRegister(instruction.RegisterTarget);

		uint32_t r = a - b;

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::ADDI(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = SIGNEXT16(instruction.Immediate);

		uint32_t r = a + b;

		if (OVERFLOW_ADD32(a, b, r)) {
			//TODO: Exception Overflow
		}

		setRegister(instruction.RegisterTarget, r);
	}

	void R3000::ADDIU(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = SIGNEXT16(instruction.Immediate);

		uint32_t r = a + b;

		setRegister(instruction.RegisterTarget, r);
	}

	void R3000::MULT(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = getRegister(instruction.RegisterTarget);

		uint64_t r = a * b;

		mHI = (r >> 32) & 0xFFFFFFFF;
		mLO = r & 0xFFFFFFFF;
	}

	void R3000::MULTU(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = getRegister(instruction.RegisterTarget);

		uint64_t r = a * b;

		mHI = (r >> 32) & 0xFFFFFFFF;
		mLO = r & 0xFFFFFFFF;
	}

	void R3000::DIV(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = getRegister(instruction.RegisterTarget);

		mHI = a % b;
		mLO = a / b;
	}

	void R3000::DIVU(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = getRegister(instruction.RegisterTarget);

		mHI = a % b;
		mLO = a / b;
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
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = SIGNEXT16(instruction.Immediate);

		uint32_t m = a + b;

		uint32_t r = load<uint32_t>(m);

		setRegister(instruction.RegisterTarget, r);
	}

	void R3000::LH(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = SIGNEXT16(instruction.Immediate);

		uint32_t m = a + b;

		uint32_t r = load<uint16_t>(m);
		r = SIGNEXT16(r);

		setRegister(instruction.RegisterTarget, r);
	}

	void R3000::LHU(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = SIGNEXT16(instruction.Immediate);

		uint32_t m = a + b;

		uint32_t r = load<uint16_t>(m);

		setRegister(instruction.RegisterTarget, r);
	}

	void R3000::LB(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = SIGNEXT16(instruction.Immediate);

		uint32_t m = a + b;

		uint32_t r = load<uint8_t>(m);
		r = SIGNEXT8(r);

		setRegister(instruction.RegisterTarget, r);
	}

	void R3000::LBU(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = SIGNEXT16(instruction.Immediate);

		uint32_t m = a + b;

		uint32_t r = load<uint8_t>(m);

		setRegister(instruction.RegisterTarget, r);
	}

	void R3000::LWL(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = SIGNEXT16(instruction.Immediate);
		uint32_t c = getRegister(instruction.RegisterTarget);

		uint32_t m = a + b;

		uint32_t am = m & ~(0x3);
		uint32_t aw = load<uint32_t>(am);

		uint32_t u = m & (0x3);
		uint32_t r = (c & (0x00FFFFFF >> (u * 8))) | (aw << (24 - (u * 8)));

		setRegister(instruction.RegisterTarget, r);
	}

	void R3000::LWR(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = SIGNEXT16(instruction.Immediate);
		uint32_t c = getRegister(instruction.RegisterTarget);

		uint32_t m = a + b;

		uint32_t am = m & ~(0x3);
		uint32_t aw = load<uint32_t>(am);

		uint32_t u = m & (0x3);
		uint32_t r = (c & (0xFFFFFF00 << ((0x3 - u) * 8))) | (aw >> (u * 8));

		setRegister(instruction.RegisterTarget, r);
	}

	void R3000::SWL(const Instruction& instruction)
	{
	}

	void R3000::SWR(const Instruction& instruction)
	{
	}

	void R3000::SB(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = SIGNEXT16(instruction.Immediate);

		uint32_t m = a + b;
		
		uint8_t v = getRegister(instruction.RegisterTarget) & 0xFF;

		store<uint8_t>(m, v);
	}

	void R3000::SH(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = SIGNEXT16(instruction.Immediate);

		uint32_t m = a + b;

		uint16_t v = getRegister(instruction.RegisterTarget) & 0xFFFF;

		store<uint16_t>(m, v);
	}

	void R3000::SW(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = SIGNEXT16(instruction.Immediate);

		uint32_t m = a + b;

		uint32_t v = getRegister(instruction.RegisterTarget);

		store<uint32_t>(m, v);
	}

	void R3000::LUI(const Instruction& instruction)
	{
		uint32_t r = instruction.Immediate << 16;
		setRegister(instruction.RegisterTarget, r);
	}

	void R3000::SLT(const Instruction& instruction)
	{
		int32_t a = getRegister(instruction.RegisterSource);
		int32_t b = getRegister(instruction.RegisterTarget);

		uint32_t r = a < b;

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::SLTU(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = getRegister(instruction.RegisterTarget);

		uint32_t r = a < b;

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::SLTI(const Instruction& instruction)
	{
		int32_t a = getRegister(instruction.RegisterSource);
		int32_t b = SIGNEXT16(instruction.Immediate);

		uint32_t r = a < b;

		setRegister(instruction.RegisterTarget, r);
	}

	void R3000::SLTIU(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = SIGNEXT16(instruction.Immediate);

		uint32_t r = a < b;

		setRegister(instruction.RegisterTarget, r);
	}

	void R3000::AND(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = getRegister(instruction.RegisterTarget);

		uint32_t r = a & b;

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::ANDI(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = instruction.Immediate;

		uint32_t r = a & b;

		setRegister(instruction.RegisterTarget, r);
	}

	void R3000::OR(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = getRegister(instruction.RegisterTarget);

		uint32_t r = a | b;

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::ORI(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = instruction.Immediate;

		uint32_t r = a | b;

		setRegister(instruction.RegisterTarget, r);
	}

	void R3000::XOR(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = getRegister(instruction.RegisterTarget);

		uint32_t r = a ^ b;

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::XORI(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = instruction.Immediate;

		uint32_t r = a ^ b;

		setRegister(instruction.RegisterTarget, r);
	}

	void R3000::NOR(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = getRegister(instruction.RegisterTarget);

		uint32_t r = ~(a | b);

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::SLL(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterTarget);
		uint32_t s = instruction.ShiftAmount;

		uint32_t r = a << s;

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::SRL(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterTarget);
		uint32_t s = instruction.ShiftAmount;

		uint32_t r = a >> s;

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::SRA(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterTarget);
		uint32_t s = instruction.ShiftAmount;

		uint32_t r = (a & 0x80000000 ? 0xFFFFFFFF << (32 - s) : 0x0) | (a >> s);

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::SLLV(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterTarget);
		uint32_t s = getRegister(instruction.RegisterSource);

		uint32_t r = a << s;

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::SRLV(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterTarget);
		uint32_t s = getRegister(instruction.RegisterSource);

		uint32_t r = a >> s;

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::SRAV(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterTarget);
		uint32_t s = getRegister(instruction.RegisterSource);

		uint32_t r = (a & 0x80000000 ? 0xFFFFFFFF << (32 - s) : 0x0) | (a >> s);

		setRegister(instruction.RegisterDestination, r);
	}

	void R3000::BEQ(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = getRegister(instruction.RegisterTarget);
		int32_t o = SIGNEXT16(instruction.Immediate) << 2;

		if (a == b) {
			mPC += o;
			mPC -= 4;
		}
	}

	void R3000::BNE(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = getRegister(instruction.RegisterTarget);
		int32_t o = SIGNEXT16(instruction.Immediate) << 2;

		if (a != b) {
			mPC += o;
			mPC -= 4;
		}
	}

	void R3000::BLTZ(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		int32_t o = SIGNEXT16(instruction.Immediate) << 2;

		if (a < 0) {
			mPC += o;
			mPC -= 4;
		}
	}

	void R3000::BLTZAL(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		int32_t o = SIGNEXT16(instruction.Immediate) << 2;

		if (a < 0) {
			setRegister(31, mPC);
			mPC += o;
			mPC -= 4;
		}
	}

	void R3000::BLEZ(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		int32_t o = SIGNEXT16(instruction.Immediate) << 2;

		if (a <= 0) {
			mPC += o;
			mPC -= 4;
		}
	}

	void R3000::BGTZ(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		int32_t o = SIGNEXT16(instruction.Immediate) << 2;

		if (a > 0) {
			mPC += o;
			mPC -= 4;
		}
	}

	void R3000::BGEZ(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		int32_t o = SIGNEXT16(instruction.Immediate) << 2;

		if (a >= 0) {
			mPC += o;
			mPC -= 4;
		}
	}

	void R3000::BGEZAL(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		int32_t o = SIGNEXT16(instruction.Immediate) << 2;

		if (a >= 0) {
			setRegister(31, mPC);
			mPC += o;
			mPC -= 4;
		}
	}

	void R3000::J(const Instruction& instruction)
	{
		uint32_t a = (mPC & 0xF0000000) | (instruction.PseudoAddress << 2);
		mPC = a;
	}

	void R3000::JR(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		mPC = a;
	}

	void R3000::JAL(const Instruction& instruction)
	{
		uint32_t a = (mPC & 0xF0000000) | (instruction.PseudoAddress << 2);
		setRegister(31, mPC);
		mPC = a;
	}

	void R3000::JALR(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		setRegister(31, mPC);
		mPC = a;
	}

	void R3000::MTC0(const Instruction& instruction)
	{
		uint32_t r = getRegister(instruction.RegisterTarget);
		setCP0Register(instruction.RegisterDestination, r);
	}

	void R3000::MFC0(const Instruction& instruction)
	{
		uint32_t r = getCP0Register(instruction.RegisterDestination);
		setRegister(instruction.RegisterTarget, r);
	}

	void esx::R3000::RFE(const Instruction& instruction)
	{
	}

	void R3000::setRegister(uint8_t index, uint32_t value)
	{
		mRegisters[index] = value;
		mRegisters[0] = 0;
	}

	uint32_t R3000::getRegister(uint8_t index)
	{
		return mRegisters[index];
	}

	void R3000::setCP0Register(uint8_t index, uint32_t value)
	{
		mCP0Registers[index] = value;
	}

	uint32_t R3000::getCP0Register(uint8_t index)
	{
		return mCP0Registers[index];
	}

}