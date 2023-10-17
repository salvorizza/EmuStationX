#include "R3000.h"

namespace esx {



	R3000::R3000()
		: BusDevice("R3000")
	{
		mPC = 0x0;

		mNextInstruction = decode(0x0);
	}

	R3000::~R3000()
	{
	}

	void R3000::clock()
	{
		mNextInstruction.Execute(mNextInstruction);
		mNextInstruction = decode(mPC);
		mPC += 4;
	}

	void R3000::write(const std::string& busName, uint32_t address, uint8_t value)
	{
	}

	uint8_t R3000::read(const std::string& busName, uint32_t address)
	{
		return 0;
	}

	uint32_t R3000::fetch(uint32_t address)
	{
		return getBus("Root")->read(address);
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

		result.Opcode = OPCODE(instruction);

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
				result.Mnemonic = std::format("j {}", result.PseudoAddress);
				result.Execute = std::bind(&R3000::J, this, std::placeholders::_1);
				break;
			}
			case 0x03: {
				result.PseudoAddress = ADDRESS(instruction);
				result.Mnemonic = std::format("jal {}", result.PseudoAddress);
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
					case 0x2B: {
						result.Mnemonic = std::format("sw {},{}({})", registersMnemonics[result.RegisterTarget], result.Immediate, registersMnemonics[result.RegisterSource]);
						result.Execute = std::bind(&R3000::SW, this, std::placeholders::_1);
						break;
					}
				}
			}

		}

		return result;
	}

	void R3000::ADDU(const Instruction& instruction)
	{
		uint32_t a = getRegister(instruction.RegisterSource);
		uint32_t b = getRegister(instruction.RegisterTarget);

		uint32_t r = a + b;

		setRegister(instruction.RegisterDestination, r);
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

}