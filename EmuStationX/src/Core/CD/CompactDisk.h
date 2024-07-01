#pragma once

#include "Base/Base.h"

namespace esx {

	constexpr U32 CD_SECTOR_DATA_SIZE = 0x800;
	constexpr U32 CD_SECTORS_PER_SECOND = 75;

	struct Sector {
		Array<U8, 12> SyncBytes;
		Array<U8, 3> Header;
		U8 Mode;
		Array<U8, 8> Subheader;
		Array<U8, CD_SECTOR_DATA_SIZE> UserData;
		Array<U8, 4> EDC;
		Array<U8, 276> ECC;
	};

	constexpr U32 CD_SECTOR_SIZE = sizeof(Sector);

	typedef Array<Sector, CD_SECTORS_PER_SECOND> Second;

	class CompactDisk {
	public:
		CompactDisk() = default;
		~CompactDisk() = default;

		virtual void seek(U8 minute, U8 second, U8 sector) = 0;
		virtual void readSector(Sector* pOutSector) = 0;

		constexpr static U32 calculateBinaryPosition(U8 minute, U8 second, U8 sector) { return ((minute * 60 + second) * CD_SECTORS_PER_SECOND + sector) * CD_SECTOR_SIZE; }
	};

}