#include "CDRWIN.h"


namespace esx {

	CDRWIN::CDRWIN(const std::filesystem::path& cuePath)
	{
		parse(cuePath);
	}

	void CDRWIN::seek(U64 seekPos)
	{
		seekPos -= calculateBinaryPosition(0, 2, 0);

		mCurrentFile = mFiles.begin();
		if (seekPos > mCurrentFile->Size) {
			mCurrentFile = mFiles.begin() + 1;
			seekPos = 0;
		}

		mCurrentFile->mStream.seekg(seekPos, mCurrentFile->mStream.beg);
		mCurrentLBA = seekPos;
	}

	void CDRWIN::readSector(Sector* pOutSector)
	{
		mCurrentFile->mStream.read(reinterpret_cast<char*>(pOutSector), sizeof(Sector));
		if (mCurrentFile->mStream.fail() == ESX_TRUE) {
			ESX_CORE_LOG_TRACE("Strano");
		}
		mCurrentLBA += sizeof(Sector);
	}

	void CDRWIN::parse(const std::filesystem::path& cuePath)
	{
		FileInputStream cueFile(cuePath);
		String line = "";
		U8 pregapMinutes = 0;
		U8 pregapSeconds = 2;
		U8 pregapSectors = 0;

		while (std::getline(cueFile, line)) {
			InputStringStream iss(line);
			String token = "";
			iss >> token;

			if (token == "FILE") {
				CDRWinFile& file = mFiles.emplace_back();
				iss >> std::quoted(file.FileName);
				auto filePath = cuePath.parent_path() / file.FileName;
				file.mStream.open(filePath, std::ios::binary);
				file.Size = std::filesystem::file_size(filePath);
			} else if (token == "TRACK") {
				CDRWinFile& file = mFiles.back();
				CDRWINTrack& track = file.Tracks.emplace_back();
				
				iss >> track.Number >> track.TrackMode;
			} else if (token == "INDEX") {
				CDRWinFile& file = mFiles.back();
				CDRWINTrack& track = file.Tracks.back();
				CDRWinIndex& index = track.Indexes.emplace_back();

				String time = "";
				Char delimiter = '\0';

				iss >> index.Number >> time;

				InputStringStream timeIss(time);

				timeIss >> index.Minute;
				timeIss >> delimiter;

				timeIss >> index.Second;
				timeIss >> delimiter;

				timeIss >> index.Sector;

				index.Minute += pregapMinutes;
				index.Second += pregapSeconds;
				index.Sector += pregapSectors;

				index.Second += index.Sector / CD_SECTORS_PER_SECOND;
				index.Sector = index.Sector % CD_SECTORS_PER_SECOND;

				index.Minute += index.Second / 60;
				index.Second = index.Second % 60;
			}
		}

		cueFile.close();
	}
}