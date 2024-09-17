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

		Sector dummySector = {};
		mCDROMDrive->ReadSector(mSeekPos - calculateBinaryPosition(0,0,1), &dummySector, 1);
	}

	void CDROMDisk::readSector(Sector* pOutSector)
	{
		mCDROMDrive->ReadSector(mSeekPos, pOutSector);
		mSeekPos += sizeof(Sector);
	}

	Optional<SubchannelQ> CDROMDisk::getCurrentSubChannelQ()
	{
		SubchannelQ subchannel = {};
		mCDROMDrive->ReadSubchannelQ(&subchannel);
		return subchannel;
	}

	MSF CDROMDisk::getTrackStart(U8 trackNumber) {
		return fromBinaryPositionToMSF(calculateBinaryPosition(0, 2, 0));
	}

}