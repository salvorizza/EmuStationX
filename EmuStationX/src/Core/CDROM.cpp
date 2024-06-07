#include "CDROM.h"

#include "InterruptControl.h"
#include "R3000.h"

namespace esx {

	CDROM::CDROM()
		: BusDevice("CDROM")
	{
		addRange(ESX_TEXT("Root"), 0x1F801800, BYTE(0x4), 0xFFFFFFFF);
		mShellOpen = ESX_FALSE;

		mStat.ShellOpen = ESX_TRUE;
		mStat.Rotating = ESX_TRUE;

		mData.resize(CD_SECTOR_SIZE * 2); //To accomodate double speed
	}

	CDROM::~CDROM()
	{
	}

	void CDROM::clock(U64 clocks)
	{
		if (!mResponses.empty() && mResponses.front().TargetCycle == clocks) {
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
		U64 clocks = getBus("Root")->getDevice<R3000>("R3000")->getClocks();

		Response response = {};
		response.Push(getStatus());
		response.Code = (responseNumber == 1) ? INT3 : ((responseNumber == 2) ? INT2 : INT1);
		response.TargetCycle = clocks + 800;
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
				U8 minute = popParameter();
				U8 second = popParameter();
				U8 sector = popParameter();

				mSeekMinute = ((minute >> 4) & 0xF) * 10 + ((minute >> 0) & 0xF);
				mSeekSecond = ((second >> 4) & 0xF) * 10 + ((second >> 0) & 0xF);
				mSeekSector = ((sector >> 4) & 0xF) * 10 + ((sector >> 0) & 0xF);

				ESX_CORE_LOG_INFO("CDROM - Setloc {}:{}:{}", mSeekMinute, mSeekSecond, mSeekSector);
				break;
			}

			case CommandType::ReadN: {
				ESX_CORE_LOG_INFO("CDROM - ReadN {}", response.Number);

				response.NumberOfResponses++;
				if (response.Number > 1) {
					Sector& sector = mSectors.emplace();
					mCD->readSector(&sector);
					mStat.Read = ESX_TRUE;
					if (response.Number > 1) {
						response.Code = INT1;
					}
					response.TargetCycle = clocks + (mMode.DoubleSpeed ? CD_READ_DELAY_2X : CD_READ_DELAY);
				}
				break;
			}

			case CommandType::Pause: {
				ESX_CORE_LOG_INFO("CDROM - Pause {}", response.Number);

				response.NumberOfResponses = 2;
				if (response.Number == 1) {
					if (mStat.Read) {
						while (!mResponses.empty()) {
							mResponses.pop();
						}
						mStat.Read = ESX_FALSE;
					}
				}
				break;
			}

			case CommandType::Init: {
				ESX_CORE_LOG_INFO("CDROM - Init {}", response.Number);

				response.NumberOfResponses = 2;
				if (response.Number == 1) {
					mStat.Rotating = ESX_TRUE;

					setMode(0x20);

					//Abort all commands
					while (!mResponses.empty()) {
						mResponses.pop();
					}
				}
				break;
			}

			case CommandType::Demute: {
				ESX_CORE_LOG_INFO("CDROM - Demute");
				break;
			}

			case CommandType::Setmode: {
				U8 parameter = popParameter();
				ESX_CORE_LOG_INFO("CDROM - Setmode {:02X}h", parameter);
				setMode(parameter);
				break;
			}

			case CommandType::SeekL: {
				ESX_CORE_LOG_INFO("CDROM - SeekL {}", response.Number);
				response.NumberOfResponses = 2;
				if (response.Number == 1) {
					mStat.Seek = ESX_FALSE;
					response.Clear();
					response.Push(getStatus());

					mCD->seek(mSeekMinute, mSeekSecond, mSeekSector);
					if (mStat.Read) {
						while (!mResponses.empty()) {
							mResponses.pop();
						}
						mStat.Read = ESX_FALSE;
					}
					while (!mSectors.empty()) {
						mSectors.pop();
					}
				} else {
					mStat.Seek = ESX_TRUE;
					mStat.Rotating = ESX_TRUE;
				}
				break;
			}

			case CommandType::Test: {
				U8 parameter = popParameter();
				ESX_CORE_LOG_INFO("CDROM - Test {:02X}h", parameter);

				switch (parameter) {
					case 0x00: {
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
						ESX_CORE_LOG_ERROR("CDROM - Test {:02X}h not handled yet", parameter);
						break;
					}
				}

				break;
			}

			case CommandType::GetID: {
				ESX_CORE_LOG_INFO("CDROM - GetID {}", response.Number);

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
				}
				else {
					response.TargetCycle = clocks + 20480;
				}
				break;
			}

			case CommandType::ReadTOC: {
				ESX_CORE_LOG_INFO("CDROM - ReadTOC {}", response.Number);

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
				ESX_CORE_LOG_ERROR("CDROM - Unsupported command {:02X}h", (U8)command);
				break;
			}
		}

		mResponses.push(response);
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
		RequestRegister requestRegister = {};

		requestRegister.WantData = (value >> 7) & 0x1;
		requestRegister.BFWR = (value >> 6) & 0x1;
		requestRegister.WantCommandStartInterrupt = (value >> 5) & 0x1;

		if (requestRegister.WantData) {
			if (mSectors.empty()) {
				ESX_CORE_LOG_TRACE("Hello");
			}
			if (mMode.WholeSector) {
				std::memcpy(mData.data(), &(mSectors.front().Header), sizeof(Sector) - 12);
				mDataSize = sizeof(Sector) - 12;
			} else {
				std::memcpy(mData.data(), &(mSectors.front().UserData), CD_SECTOR_DATA_SIZE);
				mDataSize = CD_SECTOR_DATA_SIZE;
			}
			mDataReadPointer = 0;
			mSectors.pop();
			CDROM_REG0.DataFifoEmpty = ESX_FALSE;
		}
	}

	void CDROM::setInterruptEnableRegister(U8& REG, U8 value)
	{
		REG = value & 0x1F;
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

		if (mDataReadPointer == mDataSize - 1) {
			CDROM_REG0.DataFifoEmpty = ESX_TRUE;
		}

		value = mData[mDataReadPointer++];

		return value;
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

}