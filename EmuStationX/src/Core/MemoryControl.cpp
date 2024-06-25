#include "MemoryControl.h"

namespace esx {



	MemoryControl::MemoryControl()
		: BusDevice(ESX_TEXT("MemoryControl"))
	{
		addRange(ESX_TEXT("Root"), 0x1F801000, BYTE(36), 0xFFFFFFFF);
		addRange(ESX_TEXT("Root"), 0x1F801060, BYTE(4), 0xFFFFFFFF);
		addRange(ESX_TEXT("Root"), 0xFFFE0130, BYTE(4), 0xFFFFFFFF);
		reset();
	}

	MemoryControl::~MemoryControl()
	{
	}

	void MemoryControl::store(const StringView& busName, U32 address, U32 value)
	{
		switch (address) {
			case 0x1F801000: {
				setBaseAddress(mExpansion1BaseAddress, value);
				break;
			}
			case 0x1F801004: {
				setBaseAddress(mExpansion2BaseAddress, value);
				break;
			}
			case 0x1F801008: {
				setDelaySizeRegister(mExpansion1DelaySize, value);
				break;
			}
			case 0x1F80100C: {
				setDelaySizeRegister(mExpansion3DelaySize, value);
				break;
			}
			case 0x1F801010: {
				setDelaySizeRegister(mBIOSROMDelaySize, value);
				break;
			}
			case 0x1F801014: {
				setDelaySizeRegister(mSPUDelaySize, value);
				break;
			}
			case 0x1F801018: {
				setDelaySizeRegister(mCDROMDelaySize, value);
				break;
			}
			case 0x1F80101C: {
				setDelaySizeRegister(mExpansion2DelaySize, value);
				break;
			}
			case 0x1F801020: {
				setCommonDelayRegister(value);
				break;
			}
			case 0x1F801060: {
				setRAMSizeRegister(value);
				break;
			}
			case 0xFFFE0130: {
				setCacheControlRegister(value);
				break;
			}
		}
	}

	void MemoryControl::load(const StringView& busName, U32 address, U32& output)
	{
		switch (address) {
			case 0x1F801000: {
				output = getBaseAddress(mExpansion1BaseAddress);
				break;
			}
			case 0x1F801004: {
				output = getBaseAddress(mExpansion2BaseAddress);
				break;
			}
			case 0x1F801008: {
				output = getDelaySizeRegister(mExpansion1DelaySize);
				break;
			}
			case 0x1F80100C: {
				output = getDelaySizeRegister(mExpansion3DelaySize);
				break;
			}
			case 0x1F801010: {
				output = getDelaySizeRegister(mBIOSROMDelaySize);
				break;
			}
			case 0x1F801014: {
				output = getDelaySizeRegister(mSPUDelaySize);
				break;
			}
			case 0x1F801018: {
				output = getDelaySizeRegister(mCDROMDelaySize);
				break;
			}
			case 0x1F80101C: {
				output = getDelaySizeRegister(mExpansion2DelaySize);
				break;
			}
			case 0x1F801020: {
				output = getCommonDelayRegister();
				break;
			}
			case 0x1F801060: {
				output = getRAMSizeRegister();
				break;
			}
			case 0xFFFE0130: {
				output = getCacheControlRegister();
				break;
			}
		}
	}

	void MemoryControl::reset()
	{
		mExpansion1BaseAddress = 0x00;
		mExpansion2BaseAddress = 0x00;

		mExpansion1DelaySize = {};
		mExpansion3DelaySize = {};
		mBIOSROMDelaySize = {};
		mSPUDelaySize = {};
		mCDROMDelaySize = {};
		mExpansion2DelaySize = {};

		mCommonDelay = {};

		mRAMSizeRegister = {};

		mCacheControl = {};
	}

	static constexpr Array<U32, 8> locked_region_start_map = {
		MIBI(1),
		MIBI(4),
		MIBI(2),
		0xFFFFFFFF,
		MIBI(2),
		0xFFFFFFFF,
		MIBI(4),
		0xFFFFFFFF,
	};

	U32 MemoryControl::getRAMLockedRegionStart() {
		return locked_region_start_map[(U8)mRAMSizeRegister.MemoryWindow];
	}

	static constexpr Array<Pair<U32, U32>, 8> hiz_region_range_map = {
		std::make_pair(0xFFFFFFFF,0xFFFFFFFF),
		std::make_pair(0xFFFFFFFF,0xFFFFFFFF),
		std::make_pair(MIBI(1),MIBI(2) - 1),
		std::make_pair(MIBI(4),MIBI(8) - 1),
		std::make_pair(0xFFFFFFFF,0xFFFFFFFF),
		std::make_pair(0xFFFFFFFF,0xFFFFFFFF),
		std::make_pair(MIBI(2),MIBI(4) - 1),
		std::make_pair(0xFFFFFFFF,0xFFFFFFFF)
	};

	const Pair<U32, U32>& MemoryControl::getRAMHiZRegionRange()
	{
		return hiz_region_range_map[(U8)mRAMSizeRegister.MemoryWindow];
	}

	void MemoryControl::setBaseAddress(U32& reg, U32 value)
	{
		reg = value & 0xFFFFFF;
	}

	U32 MemoryControl::getBaseAddress(U32 reg)
	{
		U32 value = 0;

		value |= (reg << 0);
		value |= 0x1F << 24;

		return value;
	}

	void MemoryControl::setDelaySizeRegister(DelaySizeRegister& reg, U32 value)
	{
		reg.WriteDelay = (value >> 0) & 0xF;
		reg.ReadDelay = (value >> 4) & 0xF;
		reg.RecoveryPeriod = (value >> 8) & 0x1;
		reg.HoldPeriod = (value >> 9) & 0x1;
		reg.FloatingPeriod = (value >> 10) & 0x1;
		reg.PreStrobePeriod = (value >> 11) & 0x1;
		reg.HalfWordDataBusWidth = (value >> 12) & 0x1;
		reg.AutoIncrement = (value >> 13) & 0x1;
		reg.Unknown = (value >> 14) & 0x3;
		reg.MemoryWindowSize = (value >> 16) & 0x1F;
		reg.DMATimingOverride = (value >> 24) & 0xF;
		reg.AddressErrorFlag = ((value >> 28) & 0x1) == 0x1 ? ESX_FALSE : reg.AddressErrorFlag;
		reg.DMATimingSelect = (value >> 29) & 0x1;
		reg.WideDMA = (value >> 30) & 0x1;
		reg.Wait = (value >> 31) & 0x1;
	}

	U32 MemoryControl::getDelaySizeRegister(DelaySizeRegister& reg)
	{
		U32 value = 0;

		value |= (reg.WriteDelay << 0);
		value |= (reg.ReadDelay << 4);
		value |= (reg.RecoveryPeriod << 8);
		value |= (reg.HoldPeriod << 9);
		value |= (reg.FloatingPeriod << 10);
		value |= (reg.PreStrobePeriod << 11);
		value |= (reg.HalfWordDataBusWidth << 12);
		value |= (reg.AutoIncrement << 13);
		value |= (reg.Unknown << 14);
		value |= (reg.MemoryWindowSize << 16);
		value |= (reg.DMATimingOverride << 24);
		value |= (reg.AddressErrorFlag << 28);
		value |= (reg.DMATimingSelect << 29);
		value |= (reg.WideDMA << 30);
		value |= (reg.Wait << 31);

		return value;
	}

	void MemoryControl::setCommonDelayRegister(U32 value)
	{
		mCommonDelay.RecoveryPeriodCycles = (value >> 0) & 0xF;
		mCommonDelay.HoldPeriodCycles = (value >> 4) & 0xF;
		mCommonDelay.FloatingReleaseCycles = (value >> 8) & 0xF;
		mCommonDelay.StrobeActiveGoingEdgeDelay = (value >> 12) & 0xF;
	}

	U32 MemoryControl::getCommonDelayRegister()
	{
		U32 value = 0;

		value |= (mCommonDelay.RecoveryPeriodCycles << 0);
		value |= (mCommonDelay.HoldPeriodCycles << 4);
		value |= (mCommonDelay.FloatingReleaseCycles << 8);
		value |= (mCommonDelay.StrobeActiveGoingEdgeDelay << 12);

		return value;
	}

	void MemoryControl::setRAMSizeRegister(U32 value)
	{
		mRAMSizeRegister.Unknown1 = (value >> 0) & 0x7;
		mRAMSizeRegister.CrashesWhenFalse = (value >> 3) & 0x1;
		mRAMSizeRegister.Unknown2 = (value >> 4) & 0x7;
		mRAMSizeRegister.DelayCODEDataFetch = (value >> 7) & 0x1;
		mRAMSizeRegister.Unknown3 = (value >> 8) & 0x1;
		mRAMSizeRegister.MemoryWindow = (MemoryWindow)((value >> 9) & 0x7);
		mRAMSizeRegister.Unknown4 = (value >> 12) & 0xF;
		mRAMSizeRegister.Unknown5 = (value >> 16) & 0xFFFF;
	}

	U32 MemoryControl::getRAMSizeRegister()
	{
		U32 value = 0;

		value |= (mRAMSizeRegister.Unknown1 << 0);
		value |= (mRAMSizeRegister.CrashesWhenFalse << 3);
		value |= (mRAMSizeRegister.Unknown2 << 4);
		value |= (mRAMSizeRegister.DelayCODEDataFetch << 7);
		value |= (mRAMSizeRegister.Unknown3 << 8);
		value |= ((U8)mRAMSizeRegister.MemoryWindow << 9);
		value |= (mRAMSizeRegister.Unknown4 << 12);
		value |= (mRAMSizeRegister.Unknown5 << 16);

		return value;
	}

	void MemoryControl::setCacheControlRegister(U32 value)
	{
		mCacheControl.Unknown1 = (value >> 0) & 0x7;
		mCacheControl.ScratchpadEnable1 = (value >> 3) & 0x1;
		mCacheControl.Unknown2 = (value >> 4) & 0x3;
		mCacheControl.Unknown3 = (value >> 6) & 0x1;
		mCacheControl.ScratchpadEnable2 = (value >> 7) & 0x1;
		mCacheControl.Unknown4 = (value >> 8) & 0x1;
		mCacheControl.Crash = (value >> 9) & 0x1;
		mCacheControl.Unknown5 = (value >> 10) & 0x1;
		mCacheControl.CodeCacheEnabled = (value >> 11) & 0x1;
		mCacheControl.Unknown6 = (value >> 12) & 0xFFFFF;
	}

	U32 MemoryControl::getCacheControlRegister()
	{
		U32 value = 0;

		value |= (mCacheControl.Unknown1 << 0);
		value |= (mCacheControl.ScratchpadEnable1 << 3);
		value |= (mCacheControl.Unknown2 << 4);
		//value |= (mCacheControl.Unknown3 << 6);
		value |= (mCacheControl.ScratchpadEnable2 << 7);
		value |= (mCacheControl.Unknown4 << 8);
		value |= (mCacheControl.Crash << 9);
		//value |= (mCacheControl.Unknown5 << 10);
		value |= (mCacheControl.CodeCacheEnabled << 11);
		value |= (mCacheControl.Unknown6 << 12);

		return value;
	}


}