#include "CDRWIN.h"


namespace esx {

	CDRWIN::CDRWIN(const std::filesystem::path& cuePath)
	{
		parse(cuePath);
	}

	void CDRWIN::seek(U8 minute, U8 second, U8 sector)
	{
		U32 seekPos = calculateBinaryPosition(minute, second - 2, sector);
		mCurrentFile = mFiles.begin();
		mCurrentFile->mStream.seekg(seekPos, mCurrentFile->mStream.beg);
	}

	void CDRWIN::readWholeSector(Sector* pOutSector, U8 numSectors)
	{
		mCurrentFile->mStream.read(reinterpret_cast<char*>(pOutSector), sizeof(Sector) * numSectors);
	}

	void CDRWIN::readWholeSeconds(Second* pOutSeconds, U8 numSeconds)
	{
		mCurrentFile->mStream.read(reinterpret_cast<char*>(pOutSeconds), sizeof(Second) * numSeconds);
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
				file.Size = file.mStream.tellg();
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