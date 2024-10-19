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

		mTrackNumber = computeCurrentTrack();

		mCurrentFile->mStream.seekg(seekPos - (mCurrentFile->Start - calculateBinaryPosition(0, 2, 0)), mCurrentFile->mStream.beg);
	}

	void CDRWIN::readSector(Sector* pOutSector)
	{
		mCurrentFile->mStream.read(reinterpret_cast<char*>(pOutSector), sizeof(Sector));
		if (mCurrentFile->mStream.fail() == ESX_TRUE) {
			ESX_CORE_LOG_TRACE("Strano");
		}
		mCurrentLBA += sizeof(Sector);

		mTrackNumber = computeCurrentTrack();
	}

	U8 CDRWIN::getLastTrack()
	{
		const auto& lastFile = mFiles[mFiles.size() - 1];
		return lastFile.Tracks[lastFile.Tracks.size() - 1].Number;
	}

	MSF CDRWIN::getTrackStart(U8 trackNumber, BIT useIndex1)
	{
		if (trackNumber == 0) {
			return fromBinaryPositionToMSF(mFiles[mFiles.size() - 1].End);
		}

		auto trackFile = getFileByTrackNumber(trackNumber);
		if (trackFile != mFiles.end()) {
			auto track = std::find_if(trackFile->Tracks.begin(), trackFile->Tracks.end(), [&](const CDRWINTrack& track) { return track.Number == trackNumber; });
			
			U32 trackIndexLBA = track->Indexes[0].lba + track->Indexes[0].pregapLba;
			if (track->Indexes.size() > 1 && useIndex1) {
				trackIndexLBA = track->Indexes[1].lba + track->Indexes[1].pregapLba;
			}

			return fromBinaryPositionToMSF(trackFile->Start + trackIndexLBA);
		}

		return {};
	}

	Vector<CDRWinFile>::iterator CDRWIN::computeFile(U64 lba)
	{
		return std::find_if(mFiles.begin(), mFiles.end(), [&](const CDRWinFile& file) { return lba >= file.Start && lba < file.End; });
	}

	Vector<CDRWinFile>::iterator esx::CDRWIN::getFileByTrackNumber(U8 track)
	{
		auto foundIt = mFiles.end();

		for (auto it = mFiles.begin(); it < mFiles.end(); ++it) {
			auto foundTrack = std::find_if(it->Tracks.begin(), it->Tracks.end(), [&](const CDRWINTrack& fileTrack) { return fileTrack.Number == track; });
			if (foundTrack != it->Tracks.end()) {
				foundIt = it;
				break;
			}
		}

		return foundIt;
	}

	U64 CDRWIN::computeGapLBA()
	{
		for (CDRWINTrack& track : mCurrentFile->Tracks) {
			for (CDRWinIndex& index : track.Indexes) {
				if ((mCurrentLBA - calculateBinaryPosition(0, 2, 0)) >= (index.lba + index.pregapLba)) {
					return index.pregapLba - calculateBinaryPosition(0, 2, 0);
				}
			}
		}
	}

	U8 CDRWIN::computeCurrentTrack()
	{
		mCurrentFile = computeFile(mCurrentLBA);
		if (mCurrentFile == mFiles.end()) {
			ESX_CORE_LOG_ERROR("LBA greater than track size");
			mCurrentFile = mFiles.begin() + 1;
			mCurrentLBA = mCurrentFile->Start + calculateBinaryPosition(0, 2, 0);
		}

		for (auto it = mCurrentFile->Tracks.begin(); it < mCurrentFile->Tracks.end();++it) {
			for (CDRWinIndex& index : it->Indexes) {
				if (mCurrentLBA >= (mCurrentFile->Start + index.lba + index.pregapLba)) {
					mCurrentTrack = it;
					return it->Number;
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
				file.Start = ((mFiles.size() == 1) ? calculateBinaryPosition(0,2,0) : mFiles[mFiles.size() - 2].End);
				file.End = file.Start + std::filesystem::file_size(filePath);
			} else if (token == "TRACK") {
				CDRWinFile& file = mFiles.back();
				CDRWINTrack& track = file.Tracks.emplace_back();
				
				iss >> track.Number >> track.TrackMode;

				track.AudioTrack = track.TrackMode == "AUDIO" ? ESX_TRUE : ESX_FALSE;
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