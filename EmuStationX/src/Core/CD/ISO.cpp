#include "ISO.h"


namespace esx {

	ISO::ISO(const std::filesystem::path& filePath)
	{
		mStream.open(filePath, std::ios::binary);
		mFileSize = std::filesystem::file_size(filePath);
	}

	void ISO::seek(U64 seekPos)
	{
		mCurrentLBA = seekPos;

		seekPos -= calculateBinaryPosition(0, 2, 0);

		mStream.seekg(seekPos, mStream.beg);
	}

	void ISO::readSector(Sector* pOutSector)
	{
		if ((mCurrentLBA - calculateBinaryPosition(0, 2, 0) + sizeof(Sector)) < mFileSize) {
			mStream.read(reinterpret_cast<char*>(pOutSector), sizeof(Sector));
			if (mStream.fail() == ESX_TRUE) {
				ESX_CORE_LOG_TRACE("Strano");
			}
			mCurrentLBA += sizeof(Sector);
		}
		else {
			ESX_CORE_LOG_ERROR("LBA Greater than file size");
		}
	}

	MSF ISO::getTrackStart(U8 trackNumber) {
		return fromBinaryPositionToMSF(calculateBinaryPosition(0, 2, 0));
	}

}