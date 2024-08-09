#pragma once

#include <cstdint>
#include <array>
#include <string>
#include <functional>
#include <format>
#include <queue>

#include "Base/Base.h"
#include "Base/Bus.h"

#include "DMA.h"
#include "GTE.h"

namespace esx {

	#define CO(x) (((x) >> 25) & 0x1)
	#define CO_N(x) (((x) >> 26) & 0x3)
	#define COP_FUNC(x) ((x) & 0x1F)

	constexpr U32 EXCEPTION_HANDLER_ADDRESS = 0x80000080;
	constexpr U32 BREAKPOINT_EXCEPTION_HANDLER_ADDRESS = 0x80000040;
	#define ADDRESS_UNALIGNED(x,type) (((x) & (sizeof(type) - 1)) != 0x0)
	#define OVERFLOW_ADD32(a,b,s) (~(((a) & 0x80000000) ^ ((b) & 0x80000000)) & (((a) & 0x80000000) ^ ((s) & 0x80000000)))
	#define OVERFLOW_SUB32(a,b,s) (((a) & 0x80000000) ^ ((b) & 0x80000000)) & (((a) & 0x80000000) ^ ((s) & 0x80000000))

	#define ESX_CORE_BIOS_LOG_TRACE(x,...) //ESX_CORE_LOG_TRACE(x,__VA_ARGS__)

	static const UnorderedMap<U32, StringView> IOMap = {
		{ 0x1F801000, "EXP1_BASE_ADDRESS" },
		{ 0x1F801004, "EXP2_BASE_ADDRESS" },
		{ 0x1F801008, "EXP1_DELAY" },
		{ 0x1F80100C, "EXP3_DELAY" },
		{ 0x1F801010, "BIOS_ROM" },
		{ 0x1F801014, "SPU_DELAY" },
		{ 0x1F801018, "CDROM_DELAY" },
		{ 0x1F80101C, "EXP2_DELAY" },
		{ 0x1F801020, "COM_DELAY" },
		{ 0x1F801040, "JOY_DATA" },
		{ 0x1F801044, "JOY_STAT" },
		{ 0x1F801048, "JOY_MODE" },
		{ 0x1F80104A, "JOY_CTRL" },
		{ 0x1F80104E, "JOY_BAUD" },
		{ 0x1F801050, "SIO_DATA" },
		{ 0x1F801054, "SIO_STAT" },
		{ 0x1F801058, "SIO_MODE" },
		{ 0x1F80105A, "SIO_CTRL" },
		{ 0x1F80105C, "SIO_MISC" },
		{ 0x1F80105E, "SIO_BAUD" },
		{ 0x1F801060, "RAM_SIZE" },
		{ 0x1F801070, "I_STAT" },
		{ 0x1F801074, "I_MASK" },
		{ 0x1F801080, "DMDECIN_MADR" },
		{ 0x1F801084, "DMDECIN_BCR" },
		{ 0x1F801088, "DMDECIN_CHCR" },
		{ 0x1F801090, "DMDECOUT_MADR" },
		{ 0x1F801094, "DMDECOUT_BCR" },
		{ 0x1F801098, "DMDECOUT_CHCR" },
		{ 0x1F8010A0, "DGPU_MADR" },
		{ 0x1F8010A4, "DGPU_BCR" },
		{ 0x1F8010A8, "DGPU_CHCR" },
		{ 0x1F8010B0, "DCDROM_MADR" },
		{ 0x1F8010B4, "DCDROM_BCR" },
		{ 0x1F8010B8, "DCDROM_CHCR" },
		{ 0x1F8010C0, "DSPU_MADR" },
		{ 0x1F8010C4, "DSPU_BCR" },
		{ 0x1F8010C8, "DSPU_CHCR" },
		{ 0x1F8010D0, "DPIO_MADR" },
		{ 0x1F8010D4, "DPIO_BCR" },
		{ 0x1F8010D8, "DPIO_CHCR" },
		{ 0x1F8010E0, "DOTC_MADR" },
		{ 0x1F8010E4, "DOTC_BCR" },
		{ 0x1F8010E8, "DOTC_CHCR" },
		{ 0x1F8010F0, "DPCR" },
		{ 0x1F8010F4, "DICR" },
		{ 0x1F8010F8, "UNKNOWN1" },
		{ 0x1F8010FC, "UNKNOWN2" },
		{ 0x1F801100, "TMR_DOTCLOCK_VAL" },
		{ 0x1F801104, "TMR_DOTCLOCK_MODE" },
		{ 0x1F801108, "TMR_DOTCLOCK_MAX" },
		{ 0x1F801110, "TMR_HRETRACE_VAL" },
		{ 0x1F801114, "TMR_HRETRACE_MODE" },
		{ 0x1F801118, "TMR_HRETRACE_MAX" },
		{ 0x1F801120, "TMR_SYSCLOCK_VAL" },
		{ 0x1F801124, "TMR_SYSCLOCK_MODE" },
		{ 0x1F801128, "TMR_SYSCLOCK_MAX" },
		{ 0x1F801800, "CDROM_REG0" },
		{ 0x1F801801, "CDROM_REG1" },
		{ 0x1F801802, "CDROM_REG2" },
		{ 0x1F801803, "CDROM_REG3" },
		{ 0x1F801810, "GPU_REG0" },
		{ 0x1F801814, "GPU_REG1" },
		{ 0x1F801820, "MDEC_REG0" },
		{ 0x1F801824, "MDEC_REG1" },
		{ 0x1F801C00, "SPU_VOICE00_LEFT_RIGHT" },
		{ 0x1F801C04, "SPU_VOICE00_ADPCM_RATE" },
		{ 0x1F801C06, "SPU_VOICE00_ADPCM_START" },
		{ 0x1F801C08, "SPU_VOICE00_ADSR" },
		{ 0x1F801C0C, "SPU_VOICE00_ADSR_CUR_VOL" },
		{ 0x1F801C0E, "SPU_VOICE00_ADPCM_REPEAT" },
		{ 0x1F801C10, "SPU_VOICE01_LEFT_RIGHT" },
		{ 0x1F801C14, "SPU_VOICE01_ADPCM_RATE" },
		{ 0x1F801C16, "SPU_VOICE01_ADPCM_START" },
		{ 0x1F801C18, "SPU_VOICE01_ADSR" },
		{ 0x1F801C1C, "SPU_VOICE01_ADSR_CUR_VOL" },
		{ 0x1F801C1E, "SPU_VOICE01_ADPCM_REPEAT" },
		{ 0x1F801C20, "SPU_VOICE02_LEFT_RIGHT" },
		{ 0x1F801C24, "SPU_VOICE02_ADPCM_RATE" },
		{ 0x1F801C26, "SPU_VOICE02_ADPCM_START" },
		{ 0x1F801C28, "SPU_VOICE02_ADSR" },
		{ 0x1F801C2C, "SPU_VOICE02_ADSR_CUR_VOL" },
		{ 0x1F801C2E, "SPU_VOICE02_ADPCM_REPEAT" },
		{ 0x1F801C30, "SPU_VOICE03_LEFT_RIGHT" },
		{ 0x1F801C34, "SPU_VOICE03_ADPCM_RATE" },
		{ 0x1F801C36, "SPU_VOICE03_ADPCM_START" },
		{ 0x1F801C38, "SPU_VOICE03_ADSR" },
		{ 0x1F801C3C, "SPU_VOICE03_ADSR_CUR_VOL" },
		{ 0x1F801C3E, "SPU_VOICE03_ADPCM_REPEAT" },
		{ 0x1F801C40, "SPU_VOICE04_LEFT_RIGHT" },
		{ 0x1F801C44, "SPU_VOICE04_ADPCM_RATE" },
		{ 0x1F801C46, "SPU_VOICE04_ADPCM_START" },
		{ 0x1F801C48, "SPU_VOICE04_ADSR" },
		{ 0x1F801C4C, "SPU_VOICE04_ADSR_CUR_VOL" },
		{ 0x1F801C4E, "SPU_VOICE04_ADPCM_REPEAT" },
		{ 0x1F801C50, "SPU_VOICE05_LEFT_RIGHT" },
		{ 0x1F801C54, "SPU_VOICE05_ADPCM_RATE" },
		{ 0x1F801C56, "SPU_VOICE05_ADPCM_START" },
		{ 0x1F801C58, "SPU_VOICE05_ADSR" },
		{ 0x1F801C5C, "SPU_VOICE05_ADSR_CUR_VOL" },
		{ 0x1F801C5E, "SPU_VOICE05_ADPCM_REPEAT" },
		{ 0x1F801C60, "SPU_VOICE06_LEFT_RIGHT" },
		{ 0x1F801C64, "SPU_VOICE06_ADPCM_RATE" },
		{ 0x1F801C66, "SPU_VOICE06_ADPCM_START" },
		{ 0x1F801C68, "SPU_VOICE06_ADSR" },
		{ 0x1F801C6C, "SPU_VOICE06_ADSR_CUR_VOL" },
		{ 0x1F801C6E, "SPU_VOICE06_ADPCM_REPEAT" },
		{ 0x1F801C70, "SPU_VOICE07_LEFT_RIGHT" },
		{ 0x1F801C74, "SPU_VOICE07_ADPCM_RATE" },
		{ 0x1F801C76, "SPU_VOICE07_ADPCM_START" },
		{ 0x1F801C78, "SPU_VOICE07_ADSR" },
		{ 0x1F801C7C, "SPU_VOICE07_ADSR_CUR_VOL" },
		{ 0x1F801C7E, "SPU_VOICE07_ADPCM_REPEAT" },
		{ 0x1F801C80, "SPU_VOICE08_LEFT_RIGHT" },
		{ 0x1F801C84, "SPU_VOICE08_ADPCM_RATE" },
		{ 0x1F801C86, "SPU_VOICE08_ADPCM_START" },
		{ 0x1F801C88, "SPU_VOICE08_ADSR" },
		{ 0x1F801C8C, "SPU_VOICE08_ADSR_CUR_VOL" },
		{ 0x1F801C8E, "SPU_VOICE08_ADPCM_REPEAT" },
		{ 0x1F801C90, "SPU_VOICE09_LEFT_RIGHT" },
		{ 0x1F801C94, "SPU_VOICE09_ADPCM_RATE" },
		{ 0x1F801C96, "SPU_VOICE09_ADPCM_START" },
		{ 0x1F801C98, "SPU_VOICE09_ADSR" },
		{ 0x1F801C9C, "SPU_VOICE09_ADSR_CUR_VOL" },
		{ 0x1F801C9E, "SPU_VOICE09_ADPCM_REPEAT" },
		{ 0x1F801CA0, "SPU_VOICE10_LEFT_RIGHT" },
		{ 0x1F801CA4, "SPU_VOICE10_ADPCM_RATE" },
		{ 0x1F801CA6, "SPU_VOICE10_ADPCM_START" },
		{ 0x1F801CA8, "SPU_VOICE10_ADSR" },
		{ 0x1F801CAC, "SPU_VOICE10_ADSR_CUR_VOL" },
		{ 0x1F801CAE, "SPU_VOICE10_ADPCM_REPEAT" },
		{ 0x1F801CB0, "SPU_VOICE11_LEFT_RIGHT" },
		{ 0x1F801CB4, "SPU_VOICE11_ADPCM_RATE" },
		{ 0x1F801CB6, "SPU_VOICE11_ADPCM_START" },
		{ 0x1F801CB8, "SPU_VOICE11_ADSR" },
		{ 0x1F801CBC, "SPU_VOICE11_ADSR_CUR_VOL" },
		{ 0x1F801CBE, "SPU_VOICE11_ADPCM_REPEAT" },
		{ 0x1F801CC0, "SPU_VOICE12_LEFT_RIGHT" },
		{ 0x1F801CC4, "SPU_VOICE12_ADPCM_RATE" },
		{ 0x1F801CC6, "SPU_VOICE12_ADPCM_START" },
		{ 0x1F801CC8, "SPU_VOICE12_ADSR" },
		{ 0x1F801CCC, "SPU_VOICE12_ADSR_CUR_VOL" },
		{ 0x1F801CCE, "SPU_VOICE12_ADPCM_REPEAT" },
		{ 0x1F801CD0, "SPU_VOICE13_LEFT_RIGHT" },
		{ 0x1F801CD4, "SPU_VOICE13_ADPCM_RATE" },
		{ 0x1F801CD6, "SPU_VOICE13_ADPCM_START" },
		{ 0x1F801CD8, "SPU_VOICE13_ADSR" },
		{ 0x1F801CDC, "SPU_VOICE13_ADSR_CUR_VOL" },
		{ 0x1F801CDE, "SPU_VOICE13_ADPCM_REPEAT" },
		{ 0x1F801CE0, "SPU_VOICE14_LEFT_RIGHT" },
		{ 0x1F801CE4, "SPU_VOICE14_ADPCM_RATE" },
		{ 0x1F801CE6, "SPU_VOICE14_ADPCM_START" },
		{ 0x1F801CE8, "SPU_VOICE14_ADSR" },
		{ 0x1F801CEC, "SPU_VOICE14_ADSR_CUR_VOL" },
		{ 0x1F801CEE, "SPU_VOICE14_ADPCM_REPEAT" },
		{ 0x1F801CF0, "SPU_VOICE15_LEFT_RIGHT" },
		{ 0x1F801CF4, "SPU_VOICE15_ADPCM_RATE" },
		{ 0x1F801CF6, "SPU_VOICE15_ADPCM_START" },
		{ 0x1F801CF8, "SPU_VOICE15_ADSR" },
		{ 0x1F801CFC, "SPU_VOICE15_ADSR_CUR_VOL" },
		{ 0x1F801CFE, "SPU_VOICE15_ADPCM_REPEAT" },
		{ 0x1F801D00, "SPU_VOICE16_LEFT_RIGHT" },
		{ 0x1F801D04, "SPU_VOICE16_ADPCM_RATE" },
		{ 0x1F801D06, "SPU_VOICE16_ADPCM_START" },
		{ 0x1F801D08, "SPU_VOICE16_ADSR" },
		{ 0x1F801D0C, "SPU_VOICE16_ADSR_CUR_VOL" },
		{ 0x1F801D0E, "SPU_VOICE16_ADPCM_REPEAT" },
		{ 0x1F801D10, "SPU_VOICE17_LEFT_RIGHT" },
		{ 0x1F801D14, "SPU_VOICE17_ADPCM_RATE" },
		{ 0x1F801D16, "SPU_VOICE17_ADPCM_START" },
		{ 0x1F801D18, "SPU_VOICE17_ADSR" },
		{ 0x1F801D1C, "SPU_VOICE17_ADSR_CUR_VOL" },
		{ 0x1F801D1E, "SPU_VOICE17_ADPCM_REPEAT" },
		{ 0x1F801D20, "SPU_VOICE18_LEFT_RIGHT" },
		{ 0x1F801D24, "SPU_VOICE18_ADPCM_RATE" },
		{ 0x1F801D26, "SPU_VOICE18_ADPCM_START" },
		{ 0x1F801D28, "SPU_VOICE18_ADSR" },
		{ 0x1F801D2C, "SPU_VOICE18_ADSR_CUR_VOL" },
		{ 0x1F801D2E, "SPU_VOICE18_ADPCM_REPEAT" },
		{ 0x1F801D30, "SPU_VOICE19_LEFT_RIGHT" },
		{ 0x1F801D34, "SPU_VOICE19_ADPCM_RATE" },
		{ 0x1F801D36, "SPU_VOICE19_ADPCM_START" },
		{ 0x1F801D38, "SPU_VOICE19_ADSR" },
		{ 0x1F801D3C, "SPU_VOICE19_ADSR_CUR_VOL" },
		{ 0x1F801D3E, "SPU_VOICE19_ADPCM_REPEAT" },
		{ 0x1F801D40, "SPU_VOICE20_LEFT_RIGHT" },
		{ 0x1F801D44, "SPU_VOICE20_ADPCM_RATE" },
		{ 0x1F801D46, "SPU_VOICE20_ADPCM_START" },
		{ 0x1F801D48, "SPU_VOICE20_ADSR" },
		{ 0x1F801D4C, "SPU_VOICE20_ADSR_CUR_VOL" },
		{ 0x1F801D4E, "SPU_VOICE20_ADPCM_REPEAT" },
		{ 0x1F801D50, "SPU_VOICE21_LEFT_RIGHT" },
		{ 0x1F801D54, "SPU_VOICE21_ADPCM_RATE" },
		{ 0x1F801D56, "SPU_VOICE21_ADPCM_START" },
		{ 0x1F801D58, "SPU_VOICE21_ADSR" },
		{ 0x1F801D5C, "SPU_VOICE21_ADSR_CUR_VOL" },
		{ 0x1F801D5E, "SPU_VOICE21_ADPCM_REPEAT" },
		{ 0x1F801D60, "SPU_VOICE22_LEFT_RIGHT" },
		{ 0x1F801D64, "SPU_VOICE22_ADPCM_RATE" },
		{ 0x1F801D66, "SPU_VOICE22_ADPCM_START" },
		{ 0x1F801D68, "SPU_VOICE22_ADSR" },
		{ 0x1F801D6C, "SPU_VOICE22_ADSR_CUR_VOL" },
		{ 0x1F801D6E, "SPU_VOICE22_ADPCM_REPEAT" },
		{ 0x1F801D70, "SPU_VOICE23_LEFT_RIGHT" },
		{ 0x1F801D74, "SPU_VOICE23_ADPCM_RATE" },
		{ 0x1F801D76, "SPU_VOICE23_ADPCM_START" },
		{ 0x1F801D78, "SPU_VOICE23_ADSR" },
		{ 0x1F801D7C, "SPU_VOICE23_ADSR_CUR_VOL" },
		{ 0x1F801D7E, "SPU_VOICE23_ADPCM_REPEAT" },
		{ 0x1F801D80, "SPU_MAIN_VOLUME_LEFT_RIGHT" },
		{ 0x1F801D84, "SPU_REVERB_OUTPUT_VOLUME_LEFT_RIGHT" },
		{ 0x1F801D88, "SPU_VOICE_KEY_ON" },
		{ 0x1F801D8C, "SPU_VOICE_KEY_OFF" },
		{ 0x1F801D90, "SPU_VOICE_FM" },
		{ 0x1F801D94, "SPU_VOICE_NOISE" },
		{ 0x1F801D98, "SPU_VOICE_REVERB" },
		{ 0x1F801D9C, "SPU_VOICE_ON_OFF" },
		{ 0x1F801DA0, "UNKNOWN3" },
		{ 0x1F801DA2, "SPU_SOUND_RAM_REVERB_WORK" },
		{ 0x1F801DA4, "SPU_SOUND_RAM_IRQ" },
		{ 0x1F801DA6, "SPU_SOUND_RAM_DATA_TRANSFER" },
		{ 0x1F801DA8, "SPU_SOUND_RAM_FIFO" },
		{ 0x1F801DAA, "SPUCNT" },
		{ 0x1F801DAC, "SPU_TRANSFER_CONTROL" },
		{ 0x1F801DAE, "SPUSTAT" },
		{ 0x1F801DB0, "SPU_CD_VOLUME_LEFT_RIGHT" },
		{ 0x1F801DB4, "SPU_EXTERN_VOLUME_LEFT_RIGHT" },
		{ 0x1F801DB8, "SPU_CURRENT_MAIN_VOLUME_LEFT_RIGHT" },
		{ 0x1F801DC0, "SPU_DAPF1" },
		{ 0x1F801DC2, "SPU_DAPF2" },
		{ 0x1F801DC4, "SPU_VIIR" },
		{ 0x1F801DC6, "SPU_VCOMB1" },
		{ 0x1F801DC8, "SPU_VCOMB2" },
		{ 0x1F801DCA, "SPU_VCOMB3" },
		{ 0x1F801DCC, "SPU_VCOMB4" },
		{ 0x1F801DCE, "SPU_VWALL" },
		{ 0x1F801DD0, "SPU_VAPF1" },
		{ 0x1F801DD2, "SPU_VAPF2" },
		{ 0x1F801DD4, "SPU_MSAME" },
		{ 0x1F801DD8, "SPU_MCOMB1" },
		{ 0x1F801DDC, "SPU_MCOMB2" },
		{ 0x1F801DE0, "SPU_DSAME" },
		{ 0x1F801DE4, "SPU_MDIFF" },
		{ 0x1F801DE8, "SPU_MCOMB3" },
		{ 0x1F801DEC, "SPU_MCOMB4" },
		{ 0x1F801DF0, "SPU_DDIFF" },
		{ 0x1F801DF4, "SPU_MAPF1" },
		{ 0x1F801DF8, "SPU_MAPF2" },
		{ 0x1F801DFC, "SPU_VIN" },
		{ 0x1F801E00, "SPU_VOICE0_CURRENT_VOLUME_LEFT_RIGHT" },
		{ 0x1F801E04, "SPU_VOICE1_CURRENT_VOLUME_LEFT_RIGHT" },
		{ 0x1F801E08, "SPU_VOICE2_CURRENT_VOLUME_LEFT_RIGHT" },
		{ 0x1F801E0C, "SPU_VOICE3_CURRENT_VOLUME_LEFT_RIGHT" },
		{ 0x1F801E10, "SPU_VOICE4_CURRENT_VOLUME_LEFT_RIGHT" },
		{ 0x1F801E14, "SPU_VOICE5_CURRENT_VOLUME_LEFT_RIGHT" },
		{ 0x1F801E18, "SPU_VOICE6_CURRENT_VOLUME_LEFT_RIGHT" },
		{ 0x1F801E1C, "SPU_VOICE7_CURRENT_VOLUME_LEFT_RIGHT" },
		{ 0x1F801E20, "SPU_VOICE8_CURRENT_VOLUME_LEFT_RIGHT" },
		{ 0x1F801E24, "SPU_VOICE9_CURRENT_VOLUME_LEFT_RIGHT" },
		{ 0x1F801E28, "SPU_VOICE10_CURRENT_VOLUME_LEFT_RIGHT" },
		{ 0x1F801E2C, "SPU_VOICE11_CURRENT_VOLUME_LEFT_RIGHT" },
		{ 0x1F801E30, "SPU_VOICE12_CURRENT_VOLUME_LEFT_RIGHT" },
		{ 0x1F801E34, "SPU_VOICE13_CURRENT_VOLUME_LEFT_RIGHT" },
		{ 0x1F801E38, "SPU_VOICE14_CURRENT_VOLUME_LEFT_RIGHT" },
		{ 0x1F801E3C, "SPU_VOICE15_CURRENT_VOLUME_LEFT_RIGHT" },
		{ 0x1F801E40, "SPU_VOICE16_CURRENT_VOLUME_LEFT_RIGHT" },
		{ 0x1F801E44, "SPU_VOICE17_CURRENT_VOLUME_LEFT_RIGHT" },
		{ 0x1F801E48, "SPU_VOICE18_CURRENT_VOLUME_LEFT_RIGHT" },
		{ 0x1F801E4C, "SPU_VOICE19_CURRENT_VOLUME_LEFT_RIGHT" },
		{ 0x1F801E50, "SPU_VOICE20_CURRENT_VOLUME_LEFT_RIGHT" },
		{ 0x1F801E54, "SPU_VOICE21_CURRENT_VOLUME_LEFT_RIGHT" },
		{ 0x1F801E58, "SPU_VOICE22_CURRENT_VOLUME_LEFT_RIGHT" },
		{ 0x1F801E5C, "SPU_VOICE23_CURRENT_VOLUME_LEFT_RIGHT" },
	};


	constexpr Array<U32, 8> SEGS_MASKS = {
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

	/*
	  cop0r0-r2   - N/A
	  cop0r3      - BPC - Breakpoint on execute (R/W)
	  cop0r4      - N/A
	  cop0r5      - BDA - Breakpoint on data access (R/W)
	  cop0r6      - JUMPDEST - Randomly memorized jump address (R)
	  cop0r7      - DCIC - Breakpoint control (R/W)
	  cop0r8      - BadVaddr - Bad Virtual Address (R)
	  cop0r9      - BDAM - Data Access breakpoint mask (R/W)
	  cop0r10     - N/A
	  cop0r11     - BPCM - Execute breakpoint mask (R/W)
	  cop0r12     - SR - System status register (R/W)
	  cop0r13     - CAUSE - Describes the most recently recognised exception (R)
	  cop0r14     - EPC - Return Address from Trap (R)
	  cop0r15     - PRID - Processor ID (R)
	  cop0r16-r31 - Garbage
	  cop0r32-r63 - N/A - None such (Control regs)
	*/
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
	class GPU;
	class Timer;
	class CDROM;
	class SIO;
	class InterruptControl;
	class SPU;

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

		inline U8 Opcode() const {
			return binaryInstruction >> 26;
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

		I32 ImmediateSE() const {
			return static_cast<I32>(static_cast<I16>(Immediate()));
		}

		U32 Code() const {
			return ((binaryInstruction >> 6) & 0xFFFFF);
		}

		U32 PseudoAddress() const {
			return (binaryInstruction & 0x3FFFFFF);
		}

		U32 Immediate25() const {
			return PseudoAddress();
		}

		String Mnemonic(const SharedPtr<R3000>& cpuState) const;
	};

	class CPUStatusPanel;
	class DisassemblerPanel;

	struct InstructionCache {
		U32 Word = 0;
		BIT Valid = ESX_FALSE;
	};

	struct CacheLine {
		U32 Tag = 0;
		Array<InstructionCache, 4> Instructions = {};
	};

	struct iCache {
		Array<CacheLine, 256> CacheLines = {};
	};
	
	class R3000 : public BusDevice {
	public:
		friend class CPUStatusPanel;
		friend class DisassemblerPanel;
		friend class TTYPanel;

		R3000();
		~R3000();

		virtual void init() override;
		void clock();
		U32 fetch(U32 address);
		void decode(Instruction& result, U32 instruction, U32 address, BIT suppressException = ESX_FALSE);
		virtual void reset();

		template<typename T>
		U32 load(U32 address, BIT& exception) {
			if (ADDRESS_UNALIGNED(address,T)) {
				raiseException(ExceptionType::AddressErrorLoad);
				exception = ESX_TRUE;
				return 0;
			}

			if (mDMA->isRunning()) {
				mStall = ESX_TRUE;
			}

			U32 physicalAddress = toPhysicalAddress(address);
			if (physicalAddress == 0x1CED68) {
				//ESX_CORE_LOG_TRACE("Load {:08x}", mCurrentInstruction.Address);
			}

			T output = mRootBus->load<T>(physicalAddress);

			/*if (physicalAddress >= 0x1F801000 && physicalAddress < 0x1F802000) {
				const StringView& ioName = IOMap.contains(address & ~0x1) ? IOMap.at(address & ~0x1) : IOMap.at(address & ~0x3);
				ESX_CORE_LOG_INFO("{:08x}h - I/O Read from {}[{:08x}h] value {:08x}h", mCurrentInstruction.Address, ioName, address, output);
			}*/

			return output;
		}

		template<typename T>
		void store(U32 address, U32 value) {
			if (ADDRESS_UNALIGNED(address, T)) {
				raiseException(ExceptionType::AddressErrorStore);
				return;
			}

			if (mDMA->isRunning()) {
				mStall = ESX_TRUE;
			}

			U32 physicalAddress = toPhysicalAddress(address);
			if (physicalAddress == 0x1ffeb8) {
				//ESX_CORE_LOG_TRACE("Store {:08x} value {:08x}h", mCurrentInstruction.Address, value);
			}

			/*if (physicalAddress >= 0x1F801000 && physicalAddress < 0x1F802000) {
				const StringView& ioName = IOMap.contains(address & ~0x1) ? IOMap.at(address & ~0x1) : IOMap.at(address & ~0x3);
				ESX_CORE_LOG_INFO("{:08x}h - I/O Write {:08x}h to {}[{:08x}h]", mCurrentInstruction.Address, value, ioName, address);
			}*/

			mRootBus->store<T>(physicalAddress, value);
		}

		static U32 toPhysicalAddress(U32 address) {
			return address & SEGS_MASKS[address >> 29];
		}

		static inline BIT isCacheActive(U32 address) {
			return (address & (1 << 29)) == 0;
		}

		void handleInterrupts();
		void raiseException(ExceptionType type);

		inline U64 getClocks() const { return mCycles; }

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
		void COP0();
		void COP1();
		void COP2();
		void COP3();
		void MTC0();
		void MFC0();
		void CFC2();
		void MTC2();
		void MFC2();
		void CTC2();
		void BC0F();
		void BC2F();
		void BC0T();
		void BC2T();
		void RFE();
		void LWC0();
		void LWC1();
		void LWC2();
		void LWC3();
		void SWC0();
		void SWC1();
		void SWC2();
		void SWC3();

		void NA();

		Instruction mCurrentInstruction;

		U32 getRegister(RegisterIndex index);

		void BiosPutChar(char c);
		void BiosPuts(U32 src);
		void BiosWrite(U32 fd, U32 src, U32 length);

	private:
		inline void addPendingLoad(RegisterIndex index, U32 value);
		inline void resetPendingLoad();

		void setRegister(RegisterIndex index, U32 value);

		void setCP0Register(RegisterIndex index, U32 value);
		U32 getCP0Register(RegisterIndex index);

		U32 cacheMiss(U32 address, U32 cacheLineNumber, U32 tag, U32 startIndex);

		void BiosA0(U32 callPC);
		void BiosB0(U32 callPC);
		void BiosC0(U32 callPC);

		void iCacheStore(U32 address, U32 value);
	private:
		SharedPtr<Bus> mRootBus;
		StringStream mTTY = {};

		Array<U32, 32> mRegisters;
		Array<U32, 64> mCP0Registers;
		GTE mGTE = {};

		Pair<RegisterIndex, U32> mPendingLoad;
		Pair<RegisterIndex, U32> mMemoryLoad;
		Pair<RegisterIndex, U32> mWriteBack;

		U32 mPC = 0;
		U32 mNextPC = 0;
		U32 mCurrentPC = 0;
		U32 mCallPC = 0;
		U32 mHI = 0;
		U32 mLO = 0;
		iCache mICache = {};
		BIT mStall = ESX_FALSE;

		BIT mBranch = ESX_FALSE;
		BIT mBranchSlot = ESX_FALSE;
		BIT mTookBranch = ESX_FALSE;
		BIT mTookBranchSlot = ESX_FALSE;

		float mGPUClock = 0;
		SharedPtr<GPU> mGPU;
		SharedPtr<Timer> mTimer;
		SharedPtr<CDROM> mCDROM;
		SharedPtr<SIO> mSIO0,mSIO1;
		SharedPtr<InterruptControl> mInterruptControl;
		SharedPtr<DMA> mDMA;
		SharedPtr<SPU> mSPU;

		U64 mCycles = 0;
		U64 mCyclesToWait = 0;
	};

}