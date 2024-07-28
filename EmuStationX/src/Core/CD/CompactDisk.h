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

	struct MSF {
		U8 Minute = 0;
		U8 Second = 0;
		U8 Sector = 0;
	};

	constexpr U32 CD_SECTOR_SIZE = sizeof(Sector);

	typedef Array<Sector, CD_SECTORS_PER_SECOND> Second;

	class CompactDisk {
	public:
		CompactDisk() = default;
		~CompactDisk() = default;

		virtual void seek(U64 seekPos) = 0;
		virtual void readSector(Sector* pOutSector) = 0;
		virtual U8 getLastTrack() = 0;
		virtual MSF getTrackStart(U8 trackNumber) = 0;

		constexpr static U32 calculateBinaryPosition(U8 minute, U8 second, U8 sector) { return ((minute * 60 + second) * CD_SECTORS_PER_SECOND + sector) * CD_SECTOR_SIZE; }
		constexpr static MSF fromBinaryPositionToMSF(U32 lba) {
			MSF result = {};

			U32 sectors = lba / CD_SECTOR_SIZE;

			U32 minutes = sectors / (60 * CD_SECTORS_PER_SECOND);
			sectors -= minutes * (60 * CD_SECTORS_PER_SECOND);

			U32 seconds = sectors / CD_SECTORS_PER_SECOND;
			sectors -= seconds * CD_SECTORS_PER_SECOND;

			result.Minute = minutes;
			result.Second = seconds;
			result.Sector = sectors;

			return result;
		}

		virtual U64 getCurrentPos() { return mCurrentLBA; }

	protected:
		U64 mCurrentLBA = 0x00;
	};

}