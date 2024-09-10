#include "CDROM.h"

#include "InterruptControl.h"
#include "R3000.h"

namespace esx {

	CDROM::CDROM()
		: BusDevice("CDROM")
	{
		addRange(ESX_TEXT("Root"), 0x1F801800, BYTE(0x4), 0xFFFFFFFF);
		reset();
	}

	CDROM::~CDROM()
	{
	}

	void CDROM::clock(U64 clocks)
	{
		if (!mResponses.empty() && mResponses.front().TargetCycle <= clocks) {
			auto& response = mResponses.front();
			
			mResponseSize = 0;
			mResponseReadPointer = 0;
			while (!response.Empty()) {
				pushResponse(response.Pop());
			}
			CDROM_REG3 = response.Code;
			if ((CDROM_REG3 & CDROM_REG2) == CDROM_REG3) {
				getBus("Root")->getDevice<InterruptControl>("InterruptControl")->requestInterrupt(InterruptType::CDROM, 0, 1);
			}

			mResponses.pop();

			if (mResponses.empty()) {
				CDROM_REG0.CommandTransmissionBusy = ESX_FALSE;
			}

			if (response.Number < response.NumberOfResponses) {
				command(response.CommandType, response.Number + 1);
			}
		}
	}

	void CDROM::store(const StringView& busName, U32 address, U8 value)
	{
		switch (address) {
			case 0x1F801800: {
				setIndexStatusRegister(value);
				break;
			}

			case 0x1F801801: {
				switch (CDROM_REG0.Index) {
					case 0x0: {
						command((CommandType)value);
						break;
					}

					case 0x3: {
						mRightCDOutToRightSPUIn = value;
						break;
					}

					default: {
						ESX_CORE_LOG_ERROR("CDROM - Writing to address 0x{:08X} with Index 0x{:02X} not implemented yet", address, CDROM_REG0.Index);
					}
				}
				break;
			}

			case 0x1F801802: {
				switch (CDROM_REG0.Index) {
					case 0x0: {
						pushParameter(value);
						break;
					}

					case 0x1: {
						setInterruptEnableRegister(CDROM_REG2,value);
						break;
					}

					case 0x2: {
						mLeftCDOutToRightSPUIn = value;
						break;
					}

					case 0x3: {
						mRightCDOutToLeftSPUIn = value;
						break;
					}

					default: {
						ESX_CORE_LOG_ERROR("CDROM - Writing to address 0x{:08X} with Index 0x{:02X} not implemented yet", address, CDROM_REG0.Index);
					}
				}
				break;
			}

			case 0x1F801803: {
				switch (CDROM_REG0.Index) {
					case 0x0: {
						setRequestRegister(value);
						break;
					}

					case 0x1: {
						setInterruptFlagRegister(CDROM_REG3,value);
						break;
					}

					case 0x2: {
						mLeftCDOutToLeftSPUIn = value;
						break;
					}

					case 0x3: {
						audioVolumeApplyChanges(value);
						break;
					}

					default: {
						ESX_CORE_LOG_ERROR("CDROM - Writing to address 0x{:08X} with Index 0x{:02X} not implemented yet", address, CDROM_REG0.Index);
					}
				}
				break;
			}
		}

	}

	void CDROM::load(const StringView& busName, U32 address, U8& output)
	{
		switch (address) {
			case 0x1F801800: {
				output = getIndexStatusRegister();
				break;
			}

			case 0x1F801801: {
				output = popResponse();
				break;
			}

			case 0x1F801802: {
				output = popData();
				break;
			}

			case 0x1F801803: {
				switch (CDROM_REG0.Index) {
					case 0x1:
					case 0x3: {
						output = getInterruptFlagRegister(CDROM_REG3);
						break;
					}

					case 0x0:
					case 0x2: {
						output = getInterruptEnableRegister(CDROM_REG3);
						break;
					}
				}
				break;
			}
		}

	}

	void CDROM::command(CommandType command, U32 responseNumber)
	{
		auto cpu = getBus("Root")->getDevice<R3000>("R3000");
		U64 clocks = cpu->getClocks();

		Response response = {};
		response.Push(getStatus());
		response.Code = (responseNumber == 1) ? INT3 : ((responseNumber == 2) ? INT2 : INT1);
		response.TargetCycle = clocks + 0xc4e1;
		response.CommandType = command;
		response.Number = responseNumber;
		response.NumberOfResponses = responseNumber;

		switch (command) {
			case CommandType::GetStat: {
				ESX_CORE_LOG_INFO("CDROM - GetStat");
				if (!mShellOpen) {
					mStat.ShellOpen = ESX_FALSE;
				}
				break;
			}

			case CommandType::Setloc: {
				U8 minute = fromBCD(popParameter());
				U8 second = fromBCD(popParameter());
				U8 sector = fromBCD(popParameter());

				mSeekLBA = CompactDisk::calculateBinaryPosition(minute, second, sector);

				mSetLocUnprocessed = ESX_TRUE;

				ESX_CORE_LOG_INFO("{:08x}h - CDROM - Setloc {:02d}:{:02d}:{:02d} - {:08x}h", cpu->mCurrentInstruction.Address, minute, second, sector, mSeekLBA - CompactDisk::calculateBinaryPosition(0, 2, 0));
				break;
			}
			
			case CommandType::Play: {		
				if (response.Number == 1) {
					U8 track = mParameters.size() != 0 ? popParameter() : 0;
					ESX_CORE_LOG_INFO("{:08x}h - CDROM - Play {} {}", cpu->mCurrentInstruction.Address, track, response.Number);
				} else {
					ESX_CORE_LOG_INFO("{:08x}h - CDROM - Play {}", cpu->mCurrentInstruction.Address, response.Number);
				}


				mStat.Rotating = ESX_TRUE;

				mStat.Read = ESX_FALSE;
				mStat.Play = ESX_TRUE;
				mStat.Seek = ESX_FALSE;

				mResponses = {};
				break;
			}

			case CommandType::ReadS:
			case CommandType::ReadN: {

				if (command == CommandType::ReadS) {
					ESX_CORE_LOG_INFO("{:08x}h - CDROM - ReadS {}", cpu->mCurrentInstruction.Address, response.Number);
				} else if (command == CommandType::ReadN) {
					ESX_CORE_LOG_INFO("{:08x}h - CDROM - ReadN {}", cpu->mCurrentInstruction.Address, response.Number);
				}

				mStat.Rotating = ESX_TRUE;

				if (mSetLocUnprocessed && response.Number == 1) {
					mCD->seek(mSeekLBA);

					mStat.Read = ESX_FALSE;
					mStat.Play = ESX_FALSE;
					mStat.Seek = ESX_TRUE;

					response.Clear();
					response.Push(getStatus());
					mStat.Seek = ESX_FALSE;

					mSetLocUnprocessed = ESX_FALSE;
				}

				mStat.Read = ESX_TRUE;
				mStat.Play = ESX_FALSE;
				mStat.Seek = ESX_FALSE;

				response.NumberOfResponses++;
				if (response.Number > 1) {
					mOldSector = mCurrentSector;
					mCurrentSector = mNextSector;
					mNextSector = (mNextSector + 1) % mSectors.size();

					mLastSubQ = generateSubChannelQ();
					mCD->readSector(&mSectors[mCurrentSector]);

					response.Code = INT1;
				} else {
					mResponses = {};
				}

				response.TargetCycle = clocks + (mMode.DoubleSpeed ? CD_READ_DELAY_2X : CD_READ_DELAY);
				break;
			}

			case CommandType::Stop: {
				ESX_CORE_LOG_INFO("{:08x}h - CDROM - Stop {}", cpu->mCurrentInstruction.Address, response.Number);

				response.NumberOfResponses = 2;

				if (response.Number == 1) {
					mResponses = {};
					mStat.Read = ESX_FALSE;
					response.Clear();
					response.Push(getStatus());
					mStat.Rotating = ESX_FALSE;
				}

				break;
			}

			case CommandType::Pause: {
				ESX_CORE_LOG_INFO("{:08x}h - CDROM - Pause {}", cpu->mCurrentInstruction.Address, response.Number);

				response.NumberOfResponses = 2;
				if (response.Number == 1) {
					if (mStat.Read) {
						mResponses = {};
						mStat.Read = ESX_FALSE;
					}
				}
				else {
					//response.TargetCycle = clocks + (mMode.DoubleSpeed ? 0x10bd93 : 0x21181c);
				}
				break;
			}

			case CommandType::Init: {
				ESX_CORE_LOG_INFO("{:08x}h - CDROM - Init {}", cpu->mCurrentInstruction.Address, response.Number);

				mStat.Read = ESX_FALSE;
				mStat.Play = ESX_FALSE;
				mStat.Seek = ESX_FALSE;

				response.NumberOfResponses = 2;
				if (response.Number == 1) {
					mStat.Rotating = ESX_TRUE;

					setMode(0x20);

					//Abort all commands
					mResponses = {};

					response.TargetCycle = clocks + 0x13cce;
				}
				break;
			}

			case CommandType::Mute: {
				ESX_CORE_LOG_INFO("{:08x}h - CDROM - Mute", cpu->mCurrentInstruction.Address);
				break;
			}

			case CommandType::Demute: {
				ESX_CORE_LOG_INFO("{:08x}h - CDROM - Demute", cpu->mCurrentInstruction.Address);
				break;
			}

			case CommandType::Setfilter: {
				U8 channel = popParameter();
				U8 file = popParameter();
				ESX_CORE_LOG_INFO("{:08x}h - CDROM - Setfilter {},{}", cpu->mCurrentInstruction.Address, file, channel);
				break;
			}

			case CommandType::Setmode: {
				U8 parameter = popParameter();
				ESX_CORE_LOG_INFO("{:08x}h - CDROM - Setmode {:02X}h", cpu->mCurrentInstruction.Address, parameter);
				setMode(parameter);
				break;
			}

			case CommandType::GetlocP: {
				ESX_CORE_LOG_INFO("{:08x}h - CDROM - GetlocP", cpu->mCurrentInstruction.Address);

				response.Clear();
				response.Push(mLastSubQ.Track);
				response.Push(mLastSubQ.Index);
				response.Push(mLastSubQ.Relative.Minute);
				response.Push(mLastSubQ.Relative.Second);
				response.Push(mLastSubQ.Relative.Sector);
				response.Push(mLastSubQ.Absolute.Minute);
				response.Push(mLastSubQ.Absolute.Second);
				response.Push(mLastSubQ.Absolute.Sector);
				break;
			}

			case CommandType::GetTN: {
				ESX_CORE_LOG_INFO("{:08x}h - CDROM - GetTN", cpu->mCurrentInstruction.Address);

				response.Push(toBCD(1));
				response.Push(toBCD(mCD->getLastTrack()));
				break;
			}

			case CommandType::GetTD: {
				U8 track = popParameter();
				ESX_CORE_LOG_INFO("{:08x}h - CDROM - GetTD {}", cpu->mCurrentInstruction.Address, track);

				MSF msf = mCD->getTrackStart(fromBCD(track));

				response.Push(toBCD(msf.Minute));
				response.Push(toBCD(msf.Second));
				break;
			}

			case CommandType::SeekL: {
				ESX_CORE_LOG_INFO("{:08x}h - CDROM - SeekL {}", cpu->mCurrentInstruction.Address, response.Number);
				response.NumberOfResponses = 2;
				if (response.Number == 1) {
					mStat.Read = ESX_FALSE;
					mStat.Play = ESX_FALSE;
					mStat.Seek = ESX_TRUE;

					mStat.Rotating = ESX_TRUE;
					response.Clear();
					response.Push(getStatus());
					mStat.Seek = ESX_FALSE;

					mCD->seek(mSeekLBA);
					if (mStat.Read) {
						mResponses = {};
						mStat.Read = ESX_FALSE;
					}
				}
				mSetLocUnprocessed = ESX_FALSE;
				break;
			}

			case CommandType::Test: {
				U8 parameter = popParameter();
				ESX_CORE_LOG_INFO("{:08x}h - CDROM - Test {:02X}h", cpu->mCurrentInstruction.Address, parameter);

				switch (parameter) {
					case 0x00: {
						mStat.Rotating = ESX_TRUE;
						break;
					}

					case 0x20: {
						response.Clear();
						response.Push(0x98);
						response.Push(0x06);
						response.Push(0x10);
						response.Push(0xC3);
						break;
					}

					default: {
						ESX_CORE_LOG_ERROR("{:08x}h - CDROM - Test {:02X}h not handled yet", cpu->mCurrentInstruction.Address, parameter);
						break;
					}
				}

				break;
			}

			case CommandType::GetID: {
				ESX_CORE_LOG_INFO("{:08x}h - CDROM - GetID {}", cpu->mCurrentInstruction.Address, response.Number);

				response.NumberOfResponses = 2;
				if (responseNumber == 2) {
					response.Clear();
					//mStat.IdError = ESX_TRUE;
					response.Push(getStatus());
					response.Push(0x00); //response.Push(GetIdFlagsUnlicensed | GetIdFlagsMissing);
					response.Push(GetIdDiskTypeMode2);
					response.Push(0x00);
					response.Push('S');
					response.Push('C');
					response.Push('E');
					response.Push('A');

					response.TargetCycle = clocks + 0x4a00;
				}
				break;
			}

			case CommandType::ReadTOC: {
				ESX_CORE_LOG_INFO("{:08x}h - CDROM - ReadTOC {}", cpu->mCurrentInstruction.Address, response.Number);

				response.NumberOfResponses = 2;
				if (responseNumber == 2) {
					response.Clear();
					response.Push(getStatus());
				}
				else {
					mStat.Rotating = ESX_TRUE;
					response.TargetCycle = clocks + CD_READ_DELAY * 180 / 4;
				}
				
				break;
			}

			default: {
				ESX_CORE_LOG_ERROR("{:08x}h - CDROM - Unsupported command {:02X}h", cpu->mCurrentInstruction.Address, (U8)command);
				break;
			}
		}

		CDROM_REG0.CommandTransmissionBusy = ESX_TRUE;
		mResponses.push(response);
	}

	void CDROM::audioVolumeApplyChanges(U8 value)
	{
		ApplyVolumeRegister reg = {};

		reg.MuteADPCM = (value >> 0) & 0x1;
		reg.ApplyChanges = (value >> 5) & 0x1;

		if (reg.MuteADPCM) {
			ESX_CORE_LOG_ERROR("CDROM - Mute ADPCM not implemented yet");
		}

		if (reg.ApplyChanges) {
			ESX_CORE_LOG_ERROR("CDROM - Apply changes not implemented yet");
		}
	}

	void CDROM::setIndexStatusRegister(U8 value)
	{
		CDROM_REG0.Index = (value >> 0) & 0x3;
	}

	U8 CDROM::getIndexStatusRegister()
	{
		U8 result = 0;

		result |= CDROM_REG0.Index;
		result |= (CDROM_REG0.ADPCMFifoEmpty == ESX_TRUE ? 0 : 1) << 2;
		result |= (CDROM_REG0.ParameterFifoEmpty == ESX_TRUE ? 1 : 0) << 3;
		result |= (CDROM_REG0.ParameterFifoFull == ESX_TRUE ? 0 : 1) << 4;
		result |= (CDROM_REG0.ResponseFifoEmpty == ESX_TRUE ? 0 : 1) << 5;
		result |= (CDROM_REG0.DataFifoEmpty == ESX_TRUE ? 0 : 1) << 6;
		result |= (CDROM_REG0.CommandTransmissionBusy == ESX_TRUE ? 1 : 0) << 7;

		return result;
	}

	void CDROM::setRequestRegister(U8 value)
	{
		auto cpu = getBus("Root")->getDevice<R3000>("R3000");

		RequestRegister requestRegister = {};

		requestRegister.WantData = (value >> 7) & 0x1;
		requestRegister.BFWR = (value >> 6) & 0x1;
		requestRegister.WantCommandStartInterrupt = (value >> 5) & 0x1;


		ESX_CORE_LOG_INFO("{:08x}h - CDROM - Request Register WantData => {}, BFWR => {}, WantCommandStartInterrupt => {}, CurrentSector => {:02x},{:02x},{:02x}",
			cpu->mCurrentInstruction.Address, requestRegister.WantData, requestRegister.BFWR, requestRegister.WantCommandStartInterrupt,
			mSectors[mOldSector].Header[0], mSectors[mOldSector].Header[1], mSectors[mOldSector].Header[2]);

		if (requestRegister.WantData) {
			if (CDROM_REG0.DataFifoEmpty == ESX_TRUE) {
				std::memcpy(mData.data(), &mSectors[mOldSector], mData.size());
				mDataReadPointer = mMode.WholeSector ? offsetof(Sector, Header) : offsetof(Sector, UserData);
				mDataWritePointer = mMode.WholeSector ? CD_SECTOR_SIZE : CD_SECTOR_DATA_SIZE;
				mDataWritePointer += mDataReadPointer;

				CDROM_REG0.DataFifoEmpty = ESX_FALSE;
			}
		} else {
			mDataWritePointer = mDataReadPointer = 0;
			CDROM_REG0.DataFifoEmpty = ESX_TRUE;
		}
	}

	void CDROM::setInterruptEnableRegister(U8& REG, U8 value)
	{
		REG = value & 0x1F;

		if ((CDROM_REG3 & CDROM_REG2) == CDROM_REG3) {
			getBus("Root")->getDevice<InterruptControl>("InterruptControl")->requestInterrupt(InterruptType::CDROM, 0, 1);
		}
	}

	U8 CDROM::getInterruptEnableRegister(U8 REG)
	{
		U8 result = 0;

		result |= (REG & 0x1F) << 0;
		result |= 0xE0;

		return result;
	}

	void CDROM::setInterruptFlagRegister(U8& REG, U8 value)
	{
		if (value & 0x40) {
			flushParameters();
		}

		REG &= ~value;
		REG &= 0x1F;
	}

	U8 CDROM::getInterruptFlagRegister(U8 REG)
	{
		REG |= 0xE0;
		return REG;
	}

	void CDROM::pushParameter(U8 value)
	{
		mParameters.push(value);
		CDROM_REG0.ParameterFifoEmpty = ESX_FALSE;
		CDROM_REG0.ParameterFifoFull = mParameters.size() == 16 ? ESX_TRUE : ESX_FALSE;
	}

	void CDROM::flushParameters()
	{
		while (!mParameters.empty()) mParameters.pop();
		CDROM_REG0.ParameterFifoEmpty = ESX_TRUE;
		CDROM_REG0.ParameterFifoFull = ESX_FALSE;
	}

	U8 CDROM::popParameter()
	{
		U8 value = mParameters.front();
		mParameters.pop();
		CDROM_REG0.ParameterFifoEmpty = mParameters.size() == 0 ? ESX_TRUE : ESX_FALSE;
		CDROM_REG0.ParameterFifoFull = ESX_FALSE;
		return value;
	}

	void CDROM::pushResponse(U8 value)
	{
		mResponse[mResponseSize++] = value;
		CDROM_REG0.ResponseFifoEmpty = ESX_FALSE;
	}

	U8 CDROM::popResponse()
	{
		U8 value = 0;

		if (mResponseReadPointer < mResponseSize) {
			value = mResponse[mResponseReadPointer];
		}
		mResponseReadPointer = (mResponseReadPointer + 1) % 16;

		if (mResponseReadPointer == mResponseSize || mResponseSize == 0) {
			CDROM_REG0.ResponseFifoEmpty = ESX_TRUE;
		}

		return value;
	}


	U8 CDROM::popData()
	{
		U8 value = 0;

		if (mDataReadPointer != mDataWritePointer) {
			value = mData[mDataReadPointer++];
		}

		if (mDataReadPointer == mDataWritePointer) {
			CDROM_REG0.DataFifoEmpty = ESX_TRUE;
		}

		return value;
	}

	void CDROM::reset()
	{
		CDROM_REG0 = {};
		CDROM_REG1 = 0x00;
		CDROM_REG2 = 0x00;
		CDROM_REG3 = 0x00;

		mLeftCDOutToLeftSPUIn = 0;
		mLeftCDOutToRightSPUIn = 0;
		mRightCDOutToRightSPUIn = 0;
		mRightCDOutToLeftSPUIn = 0;

		mParameters = {};

		mResponse = {}; 
		mResponseSize = 0x00; 
		mResponseReadPointer = 0x00;

		mStat = {};
		mMode = {};

		mResponses = {};

		mShellOpen = ESX_FALSE;
		mSeekLBA = 0x00;

		//Init
		mShellOpen = ESX_FALSE;

		mStat.ShellOpen = ESX_TRUE;
		mStat.Rotating = ESX_TRUE;

		std::fill(mData.begin(), mData.end(), 0x00);

		mSetLocUnprocessed = ESX_FALSE;
	}

	U8 CDROM::getStatus()
	{
		U8 value = 0;

		value |= mStat.Error << 0;
		value |= mStat.Rotating << 1;
		value |= mStat.SeekError << 2;
		value |= mStat.IdError << 3;
		value |= mStat.ShellOpen << 4;
		value |= mStat.Read << 5;
		value |= mStat.Seek << 6;
		value |= mStat.Play << 7;

		return value;
	}

	U8 CDROM::getMode()
	{
		U8 value = 0;

		value |= mMode.CDDA << 0;
		value |= mMode.AutoPause << 1;
		value |= mMode.Report << 2;
		value |= mMode.XAFilter << 3;
		value |= mMode.IgnoreBit << 4;
		value |= mMode.WholeSector << 5;
		value |= mMode.XAADPCM << 6;
		value |= mMode.DoubleSpeed << 7;

		return value;
	}

	void CDROM::setMode(U8 value)
	{
		mMode.CDDA = (value >> 0) & 0x1;
		mMode.AutoPause = (value >> 1) & 0x1;
		mMode.Report = (value >> 2) & 0x1;
		mMode.XAFilter = (value >> 3) & 0x1;
		mMode.IgnoreBit = (value >> 4) & 0x1;
		mMode.WholeSector = (value >> 5) & 0x1;
		mMode.XAADPCM = (value >> 6) & 0x1;
		mMode.DoubleSpeed = (value >> 7) & 0x1;
	}

	SubchannelQ CDROM::generateSubChannelQ()
	{
		SubchannelQ subq = {};

		U64 currentPos = mCD->getCurrentPos();

		U8 trackNumber = mCD->getTrackNumber();
		MSF trackStart = mCD->getTrackStart(trackNumber);
		U64 trackStartLBA = CompactDisk::calculateBinaryPosition(trackStart.Minute, trackStart.Second, trackStart.Sector);
		MSF absolutePos = CompactDisk::fromBinaryPositionToMSF(mCD->getCurrentPos());
		MSF relativePos = CompactDisk::fromBinaryPositionToMSF(mCD->getCurrentPos() - trackStartLBA);

		subq.Track = toBCD(trackNumber);
		subq.Index = toBCD(0x01);
		subq.Relative.Minute = toBCD(relativePos.Minute);
		subq.Relative.Second = toBCD(relativePos.Second);
		subq.Relative.Sector = toBCD(relativePos.Sector);
		subq.Absolute.Minute = toBCD(absolutePos.Minute);
		subq.Absolute.Second = toBCD(absolutePos.Second);
		subq.Absolute.Sector = toBCD(absolutePos.Sector);

		return subq;
	}

}