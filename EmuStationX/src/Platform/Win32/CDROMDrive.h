#pragma once

#include "Base/Base.h"
#include "Base/CDBase.h"

#include "Utils/LoggingSystem.h"

#include <Windows.h>
#include <ntddcdrm.h>
#include <ntddscsi.h>

namespace esx::platform {

	class CDROMDrive {
	public:
		CDROMDrive(const StringView& devicePath);
		~CDROMDrive();

		void ReadSector(U32 lbaBytes, Sector* sector, SubchannelQ* subchannel, U32 numSectors = 1);

		U8 GetFirstTrack() const { return mTOC.FirstTrack; }
		U8 GetNumTracks() const { return mTOC.LastTrack - mTOC.FirstTrack + 1; }
		U8 GetLastTrack() const { return mTOC.LastTrack; }

		static Vector<String> List();
	private:
		StringView mDevicePath;
		HANDLE mDeviceHandle = NULL;
		CDROM_TOC mTOC = { 0 };
		HANDLE mEvent = { 0 };
	};

}