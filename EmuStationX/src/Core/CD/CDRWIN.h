#pragma once

#include <filesystem>

#include "Base/Base.h"
#include "Utils/LoggingSystem.h"

#include "CompactDisk.h"

namespace esx {

	enum class CDRWINTrackMode {
		Audio,
		CDG,
		Mode1_800,
		Mode1_930,
		Mode2_920,
		Mode2_930,
		CDI_920,
		CDI_930
	};

	struct CDRWinIndex {
		I32 Number = 0;
		I32 Minute = 0;
		I32 Second = 0;
		I32 Sector = 0;
	};

	struct CDRWINTrack {
		I32 Number = 0;
		String TrackMode = "";
		Vector<CDRWinIndex> Indexes = {};
	};

	struct CDRWinFile {
		String FileName = "";
		U64 Size = 0;
		FileInputStream mStream = {};
		Vector<CDRWINTrack> Tracks = {};
	};

	class CDRWIN : public CompactDisk {
	public:
		CDRWIN(const std::filesystem::path& cuePath);
		~CDRWIN() = default;

		virtual void seek(U8 minute, U8 second, U8 sector) override;
		virtual void readSector(Sector* pOutSector) override;

	private:
		void parse(const std::filesystem::path& cuePath);

	private:
		StringView mCuePath;
		Vector<CDRWinFile> mFiles;
		Vector<CDRWinFile>::iterator mCurrentFile;
	};

}