#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {

	enum class MemoryWindow : U8 {
		Memory_1MB_Locked_7MB,
		Memory_4MB_Locked_4MB,
		Memory_1MB_HighZ_1MB_Locked_6MB,
		Memory_4MB_HighZ_4MB,
		Memory_2MB_Locked_6MB,
		Memory_8MB,
		Memory_2MB_HighZ_2MB_Locked_4MB,
		Memory_8MB_2
	};

	struct DelaySizeRegister {
		U8 WriteDelay = 0x00;
		U8 ReadDelay = 0x00;
		BIT RecoveryPeriod = ESX_FALSE;
		BIT HoldPeriod = ESX_FALSE;
		BIT FloatingPeriod = ESX_FALSE;
		BIT PreStrobePeriod = ESX_FALSE;
		BIT HalfWordDataBusWidth = ESX_FALSE;
		BIT AutoIncrement = ESX_FALSE;
		U8 Unknown = 0x00;
		U8 MemoryWindowSize = 0x00;
		U8 DMATimingOverride = ESX_FALSE;
		BIT AddressErrorFlag = ESX_FALSE;
		BIT DMATimingSelect = ESX_FALSE;
		BIT WideDMA = ESX_FALSE;
		BIT Wait = ESX_FALSE;
	};

	struct CommonDelayRegister {
		U8 RecoveryPeriodCycles = 0x00; //COM0
		U8 HoldPeriodCycles = 0x00; //COM1
		U8 FloatingReleaseCycles = 0x00; //COM2
		U8 StrobeActiveGoingEdgeDelay = 0x00; //COM3
	};

	struct RAMSizeRegister {
		U8 Unknown1 = 0x00;
		BIT CrashesWhenFalse = ESX_TRUE;
		U8 Unknown2 = 0x00;
		BIT DelayCODEDataFetch = ESX_FALSE;
		U8 Unknown3 = 0x00;
		MemoryWindow MemoryWindow = MemoryWindow::Memory_1MB_Locked_7MB;
		U8 Unknown4 = 0x00;
		U16 Unknown5 = 0x0000;
	};

	struct CacheControlRegister {
		U8 Unknown1 = 0x00;
		BIT ScratchpadEnable1 = ESX_FALSE;
		U8 Unknown2 = 0x00;
		U8 Unknown3 = 0x00;
		BIT ScratchpadEnable2 = ESX_FALSE;
		U8 Unknown4 = 0x00;
		BIT Crash = ESX_FALSE;
		U8 Unknown5 = 0x00;
		BIT CodeCacheEnabled = ESX_FALSE;
		U32 Unknown6 = 0x00;
	};

	class MemoryControl : public BusDevice {
	public:
		MemoryControl();
		~MemoryControl();

		virtual void store(const StringView& busName, U32 address, U32 value) override;

		virtual void reset() override;

		U32 getRAMLockedRegionStart();
		const Pair<U32,U32>& getRAMHiZRegionRange();

	private:
		void setBaseAddress(U32& reg, U32 value);
		U32 getBaseAddress(U32 reg);

		void setDelaySizeRegister(DelaySizeRegister& reg, U32 value);
		U32 getDelaySizeRegister(DelaySizeRegister& reg);

		void setCommonDelayRegister(U32 value);
		U32 getCommonDelayRegister();

		void setRAMSizeRegister(U32 value);
		U32 getRAMSizeRegister();

		void setCacheControlRegister(U32 value);
		U32 getCacheControlRegister();
	private:
		U32 mExpansion1BaseAddress = 0x00;
		U32 mExpansion2BaseAddress = 0x00;

		DelaySizeRegister mExpansion1DelaySize = {};
		DelaySizeRegister mExpansion3DelaySize = {};
		DelaySizeRegister mBIOSROMDelaySize = {};
		DelaySizeRegister mSPUDelaySize = {};
		DelaySizeRegister mCDROMDelaySize = {};
		DelaySizeRegister mExpansion2DelaySize = {};

		CommonDelayRegister mCommonDelay = {};

		RAMSizeRegister mRAMSizeRegister = {};

		CacheControlRegister mCacheControl = {};
	};

}