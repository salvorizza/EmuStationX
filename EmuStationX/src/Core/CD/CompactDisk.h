#pragma once

#include "Base/Base.h"
#include "Base/CDBase.h"

namespace esx {

	class CompactDisk {
	public:
		CompactDisk() = default;
		~CompactDisk() = default;

		virtual void seek(U64 seekPos) = 0;
		virtual void readSector(Sector* pOutSector) = 0;
		virtual U8 getLastTrack() = 0;
		virtual MSF getTrackStart(U8 trackNumber) = 0;
		virtual Optional<SubchannelQ> getCurrentSubChannelQ() { return {}; }

		virtual U64 getCurrentPos() { return mCurrentLBA; }
		virtual BIT isAudioTrack() { return ESX_FALSE; }

		U8 getTrackNumber() { return mTrackNumber; }

	protected:
		U64 mCurrentLBA = 0x00;
		U8 mTrackNumber = 0x00;
	};

}