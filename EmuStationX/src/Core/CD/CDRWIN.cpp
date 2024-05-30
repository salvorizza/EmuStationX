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

	void CDRWIN::readDataSector(SectorData* pOutSectorData, U8 numSectors)
	{
		for (I32 sector = 0; sector < numSectors; sector++) {
			mCurrentFile->mStream.seekg(12 + 4 + 8, mCurrentFile->mStream.cur);
			mCurrentFile->mStream.read(reinterpret_cast<char*>(pOutSectorData) + sector, sizeof(SectorData));
			mCurrentFile->mStream.seekg(280, mCurrentFile->mStream.cur);
		}
	}

	void CDRWIN::readWholeSeconds(Second* pOutSeconds, U8 numSeconds)
	{
		mCurrentFile->mStream.read(reinterpret_cast<char*>(pOutSeconds), sizeof(Second) * numSeconds);
	}

	void CDRWIN::readDataSeconds(SecondData* pOutSecondsData, U8 numSeconds)
	{
		for (I32 sector = 0; sector < CD_SECTORS_PER_SECOND * numSeconds; sector++) {
			readDataSector(reinterpret_cast<SectorData*>(pOutSecondsData) + sector, 1);
		}
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