#include "R3000.h"

namespace esx {



	R3000::R3000()
	{
	}

	R3000::~R3000()
	{
	}

	void R3000::Clock()
	{
	}

	uint32_t R3000::Fetch()
	{
		return 0;
	}

	Instruction R3000::Decode(uint32_t instruction)
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
					case 0x12: {
						result.Mnemonic = std::format("mflo {}", registersMnemonics[result.RegisterDestination]);
						result.Execute = std::bind(&R3000::MFLO, this, std::placeholders::_1);
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
				}

				break;
			}

			//J Type
			case 0x02: {
				result.PseudoAddress = ADDRESS(instruction);
				result.Mnemonic = std::format("J {:x}", result.PseudoAddress);
				result.Execute = std::bind(&R3000::J, this, std::placeholders::_1);
				break;
			}
			case 0x03: {
				result.PseudoAddress = ADDRESS(instruction);
				result.Mnemonic = std::format("JAL {:x}", result.PseudoAddress);
				result.Execute = std::bind(&R3000::JAL, this, std::placeholders::_1);
				break;
			}

		}
	}

}