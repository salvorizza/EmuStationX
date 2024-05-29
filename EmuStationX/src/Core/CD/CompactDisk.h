#pragma once

#include "Base/Base.h"

namespace esx {

	constexpr U32 CD_SECTOR_SIZE = 0x930;
	constexpr U32 CD_SECTOR_DATA_SIZE = 0x800;
	constexpr U32 CD_SECTORS_PER_SECOND = 75;

	typedef Array<U8, CD_SECTOR_SIZE> Sector;
	typedef Array<U8, CD_SECTOR_DATA_SIZE> SectorData;
	typedef Array<Sector, CD_SECTORS_PER_SECOND> Second;
	typedef Array<SectorData, CD_SECTORS_PER_SECOND> SecondData;

	class CompactDisk {
	public:
		CompactDisk() = default;
		~CompactDisk() = default;

		virtual void seek(U8 minute, U8 second, U8 sector) = 0;
		virtual void readWholeSector(Sector* pOutSector, U8 numSectors) = 0;
		virtual void readDataSector(SectorData* pOutSectorData, U8 numSectors) = 0;

		virtual void readWholeSeconds(Second* pOutSeconds,U8 numSeconds) = 0;
		virtual void readDataSeconds(SecondData* pOutSecondsData, U8 numSeconds) = 0;

		constexpr static U32 calculateBinaryPosition(U8 minute, U8 second, U8 sector) { return ((minute * 60 + second) * CD_SECTORS_PER_SECOND + sector) * CD_SECTOR_SIZE; }
	};

}