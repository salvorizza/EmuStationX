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

	struct SubchannelQ {
		U8 Track;
		U8 Index;
		MSF Relative;
		MSF Absolute;
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
		

		constexpr static U32 calculateBinaryPosition(U32 minute, U32 second, U32 sector) { return ((minute * 60 + second) * CD_SECTORS_PER_SECOND + sector) * CD_SECTOR_SIZE; }
		constexpr static MSF fromBinaryPositionToMSF(U32 binaryPosition) {
			MSF result = {};

			U32 lba = binaryPosition / CD_SECTOR_SIZE;

			result.Minute = lba / (CD_SECTORS_PER_SECOND * 60);
			result.Second = (lba % (CD_SECTORS_PER_SECOND * 60)) / CD_SECTORS_PER_SECOND;
			result.Sector = lba % CD_SECTORS_PER_SECOND;

			return result;
		}

		virtual U64 getCurrentPos() { return mCurrentLBA; }

		U8 getTrackNumber() { return mTrackNumber; }

	protected:
		U64 mCurrentLBA = 0x00;
		U8 mTrackNumber = 0x00;
	};

}