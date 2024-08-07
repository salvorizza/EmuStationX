#include "CDRWIN.h"


namespace esx {

	CDRWIN::CDRWIN(const std::filesystem::path& cuePath)
	{

		parse(cuePath);
	}

	void CDRWIN::seek(U64 seekPos)
	{
		seekPos -= calculateBinaryPosition(0, 2, 0);
		mCurrentLBA = seekPos;

		computeCurrentFile();

		mCurrentFile->mStream.seekg(seekPos, mCurrentFile->mStream.beg);
	}

	void CDRWIN::readSector(Sector* pOutSector)
	{
		if (mCurrentLBA + sizeof(Sector) < mCurrentFile->Size) {
			mCurrentFile->mStream.read(reinterpret_cast<char*>(pOutSector), sizeof(Sector));
			if (mCurrentFile->mStream.fail() == ESX_TRUE) {
				ESX_CORE_LOG_TRACE("Strano");
			}
			mCurrentLBA += sizeof(Sector);
		}
	}

	MSF CDRWIN::getTrackStart(U8 trackNumber)
	{
		//TODO: Multiple files
		if (trackNumber == 0) {
			CDRWINTrack& lastTrack = mCurrentFile->Tracks[mCurrentFile->Tracks.size() - 1];
			MSF end = CompactDisk::fromBinaryPositionToMSF(((mCurrentFile->Size / CD_SECTOR_SIZE) * CD_SECTOR_SIZE) + lastTrack.Indexes[lastTrack.Indexes.size() - 1].pregapLba);
			return end;
		}

		CDRWINTrack& track = mCurrentFile->Tracks[trackNumber - 1];
		return CompactDisk::fromBinaryPositionToMSF(track.Indexes[track.Indexes.size() - 1].lba + track.Indexes[track.Indexes.size() - 1].pregapLba);
	}

	void CDRWIN::computeCurrentFile()
	{
		mCurrentFile = mFiles.begin();
		if (mCurrentLBA > mCurrentFile->Size) {
			mCurrentFile = mFiles.begin() + 1;
			mCurrentLBA = 0;
		}
	}

	U64 CDRWIN::computeGapLBA()
	{
		for (CDRWINTrack& track : mCurrentFile->Tracks) {
			for (CDRWinIndex& index : track.Indexes) {
				if (mCurrentLBA >= index.lba) {
					return index.pregapLba - CompactDisk::calculateBinaryPosition(0, 2, 0);
				}
			}
		}
	}

	void CDRWIN::parse(const std::filesystem::path& cuePath)
	{
		FileInputStream cueFile(cuePath);
		String line = "";
		U32 pregapMinutes = 0;
		U32 pregapSeconds = 0;
		U32 pregapSectors = 0;

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
			} else if (token == "PREGAP") {
				String time = "";
				Char delimiter = '\0';
				U32 minute, second, sector;

				iss >> time;

				InputStringStream timeIss(time);

				timeIss >> minute;
				timeIss >> delimiter;

				timeIss >> second;
				timeIss >> delimiter;

				timeIss >> sector;

				pregapMinutes += minute;
				pregapSeconds += second;
				pregapSectors += sector;
			} else if (token == "INDEX") {
				CDRWinFile& file = mFiles.back();
				CDRWINTrack& track = file.Tracks.back();
				CDRWinIndex& index = track.Indexes.emplace_back();
				U32 minute, second, sector;
				String time = "";
				Char delimiter = '\0';

				iss >> index.Number >> time;

				InputStringStream timeIss(time);

				timeIss >> minute;
				timeIss >> delimiter;

				timeIss >> second;
				timeIss >> delimiter;

				timeIss >> sector;

				index.lba = CompactDisk::calculateBinaryPosition(minute, second, sector);
				index.pregapLba = CompactDisk::calculateBinaryPosition(pregapMinutes, pregapSeconds, pregapSectors);
			}
		}

		mCurrentFile = mFiles.end();

		cueFile.close();
	}
}