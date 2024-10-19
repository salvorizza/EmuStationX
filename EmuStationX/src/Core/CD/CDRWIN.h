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

		U32 lba = 0;
		U32 pregapLba= 0;
	};

	struct CDRWINTrack {
		I32 Number = 0;
		String TrackMode = "";
		BIT AudioTrack = ESX_FALSE;
		Vector<CDRWinIndex> Indexes = {};
	};

	struct CDRWinFile {
		String FileName = "";
		U64 Start = 0;
		U64 End = 0;
		FileInputStream mStream = {};
		Vector<CDRWINTrack> Tracks = {};
	};

	class CDRWIN : public CompactDisk {
	public:
		CDRWIN(const std::filesystem::path& cuePath);
		~CDRWIN() = default;

		virtual void seek(U64 seekPos) override;
		virtual void readSector(Sector* pOutSector) override;
		virtual U8 getLastTrack() override;
		virtual MSF getTrackStart(U8 trackNumber, BIT useIndex1 = ESX_FALSE) override;
		virtual BIT isAudioTrack() { return mCurrentTrack->AudioTrack; }

		Vector<CDRWinFile>::iterator computeFile(U64 lba);
		Vector<CDRWinFile>::iterator getFileByTrackNumber(U8 track);
		U64 computeGapLBA();
		U8 computeCurrentTrack();

	private:
		void parse(const std::filesystem::path& cuePath);

	private:
		StringView mCuePath;
		Vector<CDRWinFile> mFiles;
		Vector<CDRWinFile>::iterator mCurrentFile;
		Vector<CDRWINTrack>::iterator mCurrentTrack;
	};

}