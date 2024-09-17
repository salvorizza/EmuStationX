#pragma once

#include "Base/Base.h"
#include "Utils/LoggingSystem.h"

#include "CompactDisk.h"

#include "Platform/Win32/CDROMDrive.h"

namespace esx {

	class CDROMDisk : public CompactDisk {
	public:
		CDROMDisk(const SharedPtr<platform::CDROMDrive>& cdromDrive);
		~CDROMDisk() = default;

		virtual void seek(U64 seekPos) override;
		virtual void readSector(Sector* pOutSector) override;
		virtual Optional<SubchannelQ> getCurrentSubChannelQ() override;
		virtual U8 getLastTrack() override { return mCDROMDrive->GetLastTrack(); }
		virtual MSF getTrackStart(U8 trackNumber) override;

	private:
		SharedPtr<platform::CDROMDrive> mCDROMDrive;
		U64 mSeekPos = 0;
	};

}