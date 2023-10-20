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
	#define SIGNEXT8(x) ((x & 0x80) ? x | 0xFFFFFF00 : x)
	#define SIGNEXT16(x) ((x & 0x8000) ? x | 0xFFFF0000 : x)
	#define EXECPTION_HANDLER_ADDRESS 0x80000080
	#define ADDRESS_UNALIGNED(x,type) ((x & (sizeof(type) - 1)) != 0x0)
	#define OVERFLOW_ADD32(a,b,s) (~((a & 0x80000000) ^ (b & 0x80000000)) & ((a & 0x80000000) ^ (s & 0x80000000)))
	#define OVERFLOW_SUB32(a,b,s) ((a & 0x80000000) ^ (b & 0x80000000)) & ((a & 0x80000000) ^ (s & 0x80000000))

	enum class COP0Register : uint8_t {
		Index = 0, //Index into the TLB array (entry index register)
		Random = 1, //Randomly generated index into the TLB array (randomized access register)
		EntryLo0 = 2, //Low-order portion of the current TLB entry for even-number virtual pages
		EntryLo1 = 3, //Low-order portion of the TLB entry for odd-number virtual pages
		Context = 4, //Pointer to page-table entry in memory/lookup address
		PageMask = 5, //Control for variable page size in TLB entries
		Wired = 6, //Controls the number of fixed TLB entries
		BadVAddr = 8, //Stores virtual address for the most recent address related exception
		Count = 9, //Increments every time an opcode is processed
		EntryHi = 10, //High-order portion of the TLB entry
		Compare = 11, //Timer interrupt control
		Status = 12, //Process status register/Used for exception handling
		Cause = 13, //Exception cause register/Stores the type of exception that last occurred
		EPC = 14, //(ExceptionPC) 	Contains address of instruction that caused the exception
		PRId = 15, //Processor identification and revision.
		Config = 16, //Configuration Register
		LLAddr = 17, //Load linked address
		WatchLo = 18, //Watchpoint address
		WatchHi = 19, //Watchpoint control
		XContext = 20, //Context register for R4300i addressing/PTE array related
		CacheErr = 27, //Cache parity error control and status
		TagLo = 28, //Low-order portion of cache tag interface
		TagHi = 29, //High-order portion of cache tag interface (reserved)
		ErrorEPC = 30 //Error exception Program Counter
	};

	struct Instruction {
		uint32_t Address;
		uint32_t binaryInstruction;
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

		virtual void write(const std::string& busName, uint32_t address, uint8_t value) override {}
		virtual void read(const std::string& busName, uint32_t address, uint8_t& output) override {}

		virtual void write(const std::string& busName, uint32_t address, uint16_t value) override {}
		virtual void read(const std::string& busName, uint32_t address, uint16_t& output) override {}

		virtual void write(const std::string& busName, uint32_t address, uint32_t value) override {}
		virtual void read(const std::string& busName, uint32_t address, uint32_t& output) override {}

		

	private:
		uint32_t fetch(uint32_t address);
		Instruction decode(uint32_t instruction);

		template<typename T>
		T load(uint32_t address) {
			if (ADDRESS_UNALIGNED(address,T)) {
				//TODO: Exception Unaligned access
			}

			return getBus("Root")->read<T>(address);
		}

		template<typename T>
		void store(uint32_t address, T value) {
			if (ADDRESS_UNALIGNED(address, T)) {
				//TODO: Exception Unaligned access
			}

			return getBus("Root")->write<T>(address, value);
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

		//COP0
		void MTC0(const Instruction& instruction);
		void MFC0(const Instruction& instruction);
		void RFE(const Instruction& instruction);

		void setRegister(uint8_t index, uint32_t value);
		uint32_t getRegister(uint8_t index);

		void setCP0Register(uint8_t index, uint32_t value);
		uint32_t getCP0Register(uint8_t index);
	private:
		std::array<uint32_t, 32> mRegisters;
		uint32_t mPC;
		uint32_t mHI, mLO;

		Instruction mNextInstruction;

		std::array<uint32_t, 32> mCP0Registers;
	};

}