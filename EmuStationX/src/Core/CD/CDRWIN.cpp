#include "CDRWIN.h"


namespace esx {

	CDRWIN::CDRWIN(const std::filesystem::path& cuePath)
	{
		parse(cuePath);
	}

	void CDRWIN::seek(U64 seekPos)
	{
		mCurrentLBA = seekPos;

		seekPos -= calculateBinaryPosition(0, 2, 0);

		computeCurrentFile();

		mTrackNumber = computeCurrentTrack();

		mCurrentFile->mStream.seekg(seekPos, mCurrentFile->mStream.beg);
	}

	void CDRWIN::readSector(Sector* pOutSector)
	{
		if ((mCurrentLBA - calculateBinaryPosition(0, 2, 0) + sizeof(Sector)) < mCurrentFile->Size) {
			mCurrentFile->mStream.read(reinterpret_cast<char*>(pOutSector), sizeof(Sector));
			if (mCurrentFile->mStream.fail() == ESX_TRUE) {
				ESX_CORE_LOG_TRACE("Strano");
			}
			mCurrentLBA += sizeof(Sector);
		} else {
			ESX_CORE_LOG_ERROR("LBA Greater than file size");
		}

		if ((mTrackNumber + 1) < mCurrentFile->Tracks.size()) {
			auto& index = mCurrentFile->Tracks[mTrackNumber + 1].Indexes[0];
			if (mTrackNumber < (mCurrentFile->Tracks.size() - 1) && mCurrentLBA >(index.lba + index.pregapLba)) {
				mTrackNumber++;
			}
		}
	}

	MSF CDRWIN::getTrackStart(U8 trackNumber)
	{
		//TODO: Multiple files
		if (trackNumber == 0) {
			CDRWINTrack& lastTrack = mCurrentFile->Tracks[mCurrentFile->Tracks.size() - 1];
			MSF end = fromBinaryPositionToMSF(((mCurrentFile->Size / CD_SECTOR_SIZE) * CD_SECTOR_SIZE) + lastTrack.Indexes[lastTrack.Indexes.size() - 1].pregapLba);
			return end;
		}

		if (trackNumber < mCurrentFile->Tracks.size()) {
			CDRWINTrack& track = mCurrentFile->Tracks[trackNumber - 1];
			return fromBinaryPositionToMSF(track.Indexes[track.Indexes.size() - 1].lba + track.Indexes[track.Indexes.size() - 1].pregapLba);
		}

		return {};
	}

	void CDRWIN::computeCurrentFile()
	{
		mCurrentFile = mFiles.begin();
		if (mCurrentLBA > mCurrentFile->Size) {
			ESX_CORE_LOG_ERROR("LBA greater than track size");
			mCurrentFile = mFiles.begin() + 1;
			mCurrentLBA = 0;
		}
	}

	U64 CDRWIN::computeGapLBA()
	{
		for (CDRWINTrack& track : mCurrentFile->Tracks) {
			for (CDRWinIndex& index : track.Indexes) {
				if (mCurrentLBA >= (index.lba + index.pregapLba)) {
					return index.pregapLba - calculateBinaryPosition(0, 2, 0);
				}
			}
		}
	}

	U8 CDRWIN::computeCurrentTrack()
	{
		for (CDRWINTrack& track : mCurrentFile->Tracks) {
			for (CDRWinIndex& index : track.Indexes) {
				if (mCurrentLBA >= (index.lba + index.pregapLba)) {
					return track.Number;
				}
			}
		}
		return 0;
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

				index.lba = calculateBinaryPosition(minute, second, sector);
				index.pregapLba = calculateBinaryPosition(pregapMinutes, pregapSeconds, pregapSectors);
			}
		}

		mCurrentFile = mFiles.end();

		cueFile.close();
	}
}