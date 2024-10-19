#pragma once

#include <filesystem>

#include "Base/Base.h"
#include "Utils/LoggingSystem.h"

#include "CompactDisk.h"

namespace esx {

	class ISO : public CompactDisk {
	public:
		ISO(const std::filesystem::path& filePath);
		~ISO() = default;

		virtual void seek(U64 seekPos) override;
		virtual void readSector(Sector* pOutSector) override;
		virtual U8 getLastTrack() override { return 1; }
		virtual MSF getTrackStart(U8 trackNumber, BIT useIndex1 = ESX_FALSE) override;

	private:
		StringView mFilePath;
		FileInputStream mStream = {};
		U64 mFileSize = 0;
	};

}