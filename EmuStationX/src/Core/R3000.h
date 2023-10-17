#pragma once

#include <cstdint>
#include <array>
#include <string>
#include <functional>
#include <format>

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

	struct Instruction {
		uint32_t Address;
		std::string Mnemonic;
		std::function<void(const Instruction&)> Execute;

		uint8_t Opcode;
		uint8_t RegisterSource;
		uint8_t RegisterTarget;
		uint8_t RegisterDestination;
		uint8_t ShiftAmount;
		uint8_t Function;
		uint16_t Immediate;
		uint32_t PseudoAddress;
	};

	class R3000 : public BusDevice {
	public:
		R3000();
		~R3000();

		void clock();


		virtual void write(const std::string& busName, uint32_t address, uint8_t value) override;
		virtual uint8_t read(const std::string& busName, uint32_t address) override;

	private:
		uint32_t fetch(uint32_t address);
		Instruction decode(uint32_t instruction);

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


		void setRegister(uint8_t index, uint32_t value);
		uint32_t getRegister(uint8_t index);
	private:
		std::array<uint32_t, 32> mRegisters;
		uint32_t mPC;
		uint32_t mHI, mLO;

		Instruction mNextInstruction;

	};

}