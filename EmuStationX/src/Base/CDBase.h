#pragma once

#include "Base/Base.h"

namespace esx {

	constexpr U32 CD_SECTOR_DATA_SIZE = 0x800;
	constexpr U32 CD_SECTORS_PER_SECOND = 75;

	struct Sector {
		Array<U8, 12> SyncBytes;
		Array<U8, 3> Header;
		U8 Mode;
		Array<U8, 4> Subheader;
		Array<U8, 4> CopyOfSubheader;
		Array<U8, CD_SECTOR_DATA_SIZE> UserData;
		Array<U8, 4> EDC;
		Array<U8, 276> ECC;

		inline BIT IsADPCM() const { return Subheader[3] != 0; }
	};

	constexpr U32 CD_SECTOR_SIZE = sizeof(Sector);

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

	struct AudioFrame {
		I16 Left = 0, Right = 0;

		AudioFrame() = default;

		AudioFrame(I16 left, I16 right)
			: Left(left), Right(right)
		{}
	};

	struct AudioBatch {
		Array<AudioFrame, 441> Batch = {};
		U32 CurrentFrame = 0;

		BIT Complete() const {
			return CurrentFrame == Batch.size() ? ESX_TRUE : ESX_FALSE;
		}
	};

	constexpr static U8 fromBCD(U8 bcd) { return ((bcd >> 4) & 0xF) * 10 + ((bcd >> 0) & 0xF); }
	constexpr static U8 toBCD(U8 decimal) {
		U8 bcd = 0;

		if (decimal == 0) {
			return 0;
		}

		U8 bitsToShift = 0;
		while (decimal != 0) {
			U8 remainder = decimal % 10;

			bcd |= (remainder << bitsToShift);

			decimal /= 10;
			bitsToShift += 4;
		}


		return bcd;
	}


	constexpr static U32 calculateBinaryPosition(U32 minute, U32 second, U32 sector) { return ((minute * 60 + second) * CD_SECTORS_PER_SECOND + sector) * CD_SECTOR_SIZE; }
	constexpr static MSF fromBinaryPositionToMSF(U32 binaryPosition) {
		MSF result = {};

		U32 lba = binaryPosition / CD_SECTOR_SIZE;

		result.Minute = lba / (CD_SECTORS_PER_SECOND * 60);
		result.Second = (lba % (CD_SECTORS_PER_SECOND * 60)) / CD_SECTORS_PER_SECOND;
		result.Sector = lba % CD_SECTORS_PER_SECOND;

		return result;
	}
}