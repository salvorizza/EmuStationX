#include "CDROMDisk.h"


namespace esx {

	CDROMDisk::CDROMDisk(const SharedPtr<platform::CDROMDrive>& cdromDrive)
		: mCDROMDrive(cdromDrive)
	{
	}

	void CDROMDisk::seek(U64 seekPos)
	{
		mCurrentLBA = seekPos;
		mSeekPos = mCurrentLBA - calculateBinaryPosition(0, 2, 0);
	}

	void CDROMDisk::readSector(Sector* pOutSector)
	{
		mCDROMDrive->ReadSector(mSeekPos, pOutSector, &mCurrentSubChannelQ);
		mSeekPos += sizeof(Sector);
	}

	Optional<SubchannelQ> CDROMDisk::getCurrentSubChannelQ()
	{
		return mCurrentSubChannelQ;
	}

	MSF CDROMDisk::getTrackStart(U8 trackNumber) {
		return fromBinaryPositionToMSF(calculateBinaryPosition(0, 2, 0));
	}

}