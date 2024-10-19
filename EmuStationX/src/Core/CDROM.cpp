#include "CDROM.h"

#include "InterruptControl.h"
#include "R3000.h"

#include "Core/Scheduler.h"

namespace esx {

	CDROM::CDROM()
		: BusDevice("CDROM")
	{
		addRange(ESX_TEXT("Root"), 0x1F801800, BYTE(0x4), 0xFFFFFFFF);

		Scheduler::AddSchedulerEventHandler(SchedulerEventType::CDROMCommand, [&](const SchedulerEvent& ev) {
			size_t serial = ev.Read<size_t>();

			if (mResponses.contains(serial)) {
				const Response& response = mResponses.at(serial);
				if ((CDROM_REG3 & 0xF) != 0 && response.GenerateInterrupt == ESX_TRUE) {
					mQueuedResponses.push(serial);
					return;
				}

				handleResponse(serial);
			}
		});
	}

	CDROM::~CDROM()
	{
	}

	void CDROM::clock(U64 clocks)
	{
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
						mRightCDOutToRightSPUInTemp = value;
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
						mLeftCDOutToLeftSPUInTemp = value;
						break;
					}

					case 0x3: {
						mRightCDOutToLeftSPUInTemp = value;
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
						mLeftCDOutToRightSPUInTemp = value;
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

				mSeekLBA = calculateBinaryPosition(minute, second, sector) - (mMode.IgnoreBit ? calculateBinaryPosition(0,0,rand() % 4) : 0);

				mSetLocUnprocessed = ESX_TRUE;

				ESX_CORE_LOG_INFO("{:08x}h - CDROM - Setloc {:02d}:{:02d}:{:02d} - {:08x}h", cpu->mCurrentInstruction.Address, minute, second, sector, mSeekLBA - calculateBinaryPosition(0, 2, 0));
				break;
			}
			
			case CommandType::Play: {		
				if (response.Number == 1) {
					U8 track = mParameters.size() != 0 ? popParameter() : 0;

					ESX_CORE_LOG_INFO("{:08x}h - CDROM - Play {} {}", cpu->mCurrentInstruction.Address, track, response.Number);

					mAudioFrames.clear();
					CDROM_REG0.ADPCMFifoEmpty = ESX_TRUE;

					if (track == 0) {
						if (mSetLocUnprocessed) {
							mCD->seek(mSeekLBA);

							mStat.Read = ESX_FALSE;
							mStat.Play = ESX_FALSE;
							mStat.Seek = ESX_TRUE;

							response.Clear();
							response.Push(getStatus());
							mStat.Seek = ESX_FALSE;

							mSetLocUnprocessed = ESX_FALSE;
						}
					} else {
						MSF msf = mCD->getTrackStart(track);
						mSeekLBA = calculateBinaryPosition(msf.Minute, msf.Second, msf.Sector);
						mCD->seek(mSeekLBA);

						mStat.Read = ESX_FALSE;
						mStat.Play = ESX_FALSE;
						mStat.Seek = ESX_TRUE;

						response.Clear();
						response.Push(getStatus());
						mStat.Seek = ESX_FALSE;
					}
				}

				mStat.Rotating = ESX_TRUE;

				mStat.Read = ESX_FALSE;
				mStat.Play = ESX_TRUE;
				mStat.Seek = ESX_FALSE;

				response.NumberOfResponses++;
				if (response.Number > 1) {
					

					mOldSector = mCurrentSector;
					mCurrentSector = mNextSector;
					mNextSector = (mNextSector + 1) % mSectors.size();

					Sector& currentSector = mSectors[mCurrentSector];

					mCD->readSector(&currentSector);

					U32 peek = 0;
					Span<AudioFrame> buffer(reinterpret_cast<AudioFrame*>(&currentSector), reinterpret_cast<AudioFrame*>(reinterpret_cast<U8*>(&currentSector) + sizeof(Sector)));

					/*I32 numSamples = buffer.size();
					I32 remainingSpace = (44100 * 2) - mAudioFrames.size();
					if (remainingSpace < numSamples) {
						I32 samplesToDiscard = numSamples - remainingSpace;
						for (I32 i = 0; i < samplesToDiscard; i++) {
							mAudioFrames.pop_front();
						}
					}*/

					for (AudioFrame& frame : buffer) {
						Output44100Hz(frame);

						peek = std::max<U32>(mPlayPeekRight ? std::abs(frame.Right) : std::abs(frame.Left), peek);
					}
					peek = std::min<U32>(peek, 0x7FFF) | (mPlayPeekRight ? 0x8000 : 0x0000);
					mPlayPeekRight = !mPlayPeekRight;

					response.GenerateInterrupt = ESX_FALSE;
					if (mMode.Report) {
						if ((mLastSubQ.Absolute.Sector & 0xF) == 0) {
							BIT isEven = ((mLastSubQ.Absolute.Sector >> 4) % 2) == 0;
							response.Clear();
							response.Push(getStatus());
							response.Push(mCD->getTrackNumber());
							response.Push(0x01);
							if (isEven) {
								response.Push(mLastSubQ.Absolute.Minute);
								response.Push(mLastSubQ.Absolute.Second);
								response.Push(mLastSubQ.Absolute.Sector);
							}
							else {
								response.Push(mLastSubQ.Relative.Minute);
								response.Push(mLastSubQ.Relative.Second | 0x80);
								response.Push(mLastSubQ.Relative.Sector);
							}
							response.Push(static_cast<U8>(peek & 0xFF));
							response.Push(static_cast<U8>((peek >> 8) & 0xFF));

							response.GenerateInterrupt = ESX_TRUE;
							response.Code = INT1;
						}
					}

					SubchannelQ newSubQ = mCD->getCurrentSubChannelQ().value_or(generateSubChannelQ());
					if (mMode.AutoPause && newSubQ.Track != mLastSubQ.Track && response.Number > 2) {
						response.NumberOfResponses--;
						mStat.Play = ESX_FALSE;

						response.Clear();
						response.Push(getStatus());
						response.Code = INT4;
						response.GenerateInterrupt = ESX_TRUE;
					}
					mLastSubQ = newSubQ;
				}
				else {
					AbortRead();
				}

				response.TargetCycle = clocks + (mMode.DoubleSpeed ? CD_READ_DELAY_2X : CD_READ_DELAY);
				break;
			}

			case CommandType::ReadS:
			case CommandType::ReadN: {
				if (command == CommandType::ReadS && response.Number == 1) {
					ESX_CORE_LOG_INFO("{:08x}h - CDROM - ReadS {}", cpu->mCurrentInstruction.Address, response.Number);
				} else if (command == CommandType::ReadN && response.Number == 1) {
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

					Sector& currentSector = mSectors[mCurrentSector];


					mCD->readSector(&currentSector);
					mLastSubQ = mCD->getCurrentSubChannelQ().value_or(generateSubChannelQ());

					U8 audioRealTimeMask = SubmodeFlagAudio | SubmodeFlagRealTime;
					BIT rejectADPCM = (
						mCD->isAudioTrack() == ESX_TRUE ||
						currentSector.Mode != 2 ||
						mMode.XAADPCM == ESX_FALSE ||
						(mMode.XAFilter == ESX_TRUE && (currentSector.Subheader[0] != mXAFilterFile || (currentSector.Subheader[1] & 0x1F) != mXAFilterChannel)) ||
						((currentSector.Subheader[2] & audioRealTimeMask) != audioRealTimeMask)
					);

					if (rejectADPCM == ESX_FALSE) {
						decodeXAADPCMSector(currentSector);

						response.GenerateInterrupt = ESX_FALSE;
					} else {
						if (mMode.XAFilter == ESX_TRUE && ((currentSector.Subheader[2] & audioRealTimeMask) == audioRealTimeMask)) {
							response.GenerateInterrupt = ESX_FALSE;
						}
					}

					response.Code = INT1;
				} else {
					AbortRead();
				}

				response.TargetCycle = clocks + (mMode.DoubleSpeed ? CD_READ_DELAY_2X : CD_READ_DELAY);
				break;
			}

			case CommandType::Stop: {
				ESX_CORE_LOG_INFO("{:08x}h - CDROM - Stop {}", cpu->mCurrentInstruction.Address, response.Number);

				response.NumberOfResponses = 2;

				if (response.Number == 1) {
					AbortRead();
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
					mAudioFrames.clear();
					CDROM_REG0.ADPCMFifoEmpty = ESX_TRUE;

					if (mStat.Read) {
						AbortRead();
						mStat.Read = ESX_FALSE;
					}
					if (mStat.Play) {
						Abort(CommandType::Play);
						mStat.Play = ESX_FALSE;
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
				mAudioStreamingMuteCDDA = mAudioStreamingMuteADPCM = ESX_TRUE;
				break;
			}

			case CommandType::Demute: {
				ESX_CORE_LOG_INFO("{:08x}h - CDROM - Demute", cpu->mCurrentInstruction.Address);
				mAudioStreamingMuteCDDA = mAudioStreamingMuteADPCM = ESX_FALSE;
				break;
			}

			case CommandType::Setfilter: {
				U8 file = popParameter();
				U8 channel = popParameter();
				ESX_CORE_LOG_INFO("{:08x}h - CDROM - Setfilter {},{}", cpu->mCurrentInstruction.Address, file, channel);

				mXAFilterFile = file;
				mXAFilterChannel = channel;
				break;
			}

			case CommandType::Setmode: {
				U8 parameter = popParameter();
				ESX_CORE_LOG_INFO("{:08x}h - CDROM - Setmode {:02X}h", cpu->mCurrentInstruction.Address, parameter);
				setMode(parameter);
				break;
			}

			case CommandType::GetlocL: {
				ESX_CORE_LOG_INFO("{:08x}h - CDROM - GetlocL", cpu->mCurrentInstruction.Address);

				response.Clear();
				U8* begin = reinterpret_cast<U8*>(&(mSectors[mCurrentSector].Header[0]));
				for (U8* it = begin; it < (begin + 8); it++) {
					response.Push(*it);
				}

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

				ESX_CORE_LOG_TRACE("SubQ {:02x},{:02x},{:02x},{:02x},{:02x},{:02x},{:02x},{:02x}",
					mLastSubQ.Track, mLastSubQ.Index, mLastSubQ.Relative.Minute, mLastSubQ.Relative.Second, mLastSubQ.Relative.Sector,
					mLastSubQ.Absolute.Minute, mLastSubQ.Absolute.Second, mLastSubQ.Absolute.Sector
				);

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

				MSF msf = mCD->getTrackStart(fromBCD(track), ESX_TRUE);

				response.Push(toBCD(msf.Minute));
				response.Push(toBCD(msf.Second));

				ESX_CORE_LOG_INFO("{:08x}h - CDROM - GetTD {:02X} {:02X} {:02X}", cpu->mCurrentInstruction.Address, track, response.Data[1], response.Data[2]);

				break;
			}

			case CommandType::SeekL:
			case CommandType::SeekP: {
				if (command == CommandType::SeekL) {
					ESX_CORE_LOG_INFO("{:08x}h - CDROM - SeekL {}", cpu->mCurrentInstruction.Address, response.Number);
				} else if (command == CommandType::SeekP) {
					ESX_CORE_LOG_INFO("{:08x}h - CDROM - SeekP {}", cpu->mCurrentInstruction.Address, response.Number);
				}

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
						AbortRead();
						mStat.Read = ESX_FALSE;
					}
				}
				mSetLocUnprocessed = ESX_FALSE;

				mLastSubQ = mCD->getCurrentSubChannelQ().value_or(generateSubChannelQ());
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
					response.Push(0x00); 
					//response.Push(GetIdFlagsUnlicensed | GetIdFlagsMissing);
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

		mResponses[mResponsesSerial++] = response;

		SchedulerEvent cdromEvent = {
			.Type = SchedulerEventType::CDROMCommand,
			.ClockStart = clocks,
			.ClockTarget = response.TargetCycle
		};
		cdromEvent.Write<size_t>(mResponsesSerial - 1);
		Scheduler::ScheduleEvent(cdromEvent);

	}

	void CDROM::handleResponse(U64 serial)
	{
		if (mResponses.contains(serial)) {
			Response& response = mResponses.at(serial);

			if (response.GenerateInterrupt) {
				mResponseSize = 0;
				mResponseReadPointer = 0;
				while (!response.Empty()) {
					pushResponse(response.Pop());
				}
				CDROM_REG3 = (CDROM_REG3 & 0xF0) | response.Code;
				if ((CDROM_REG3 & CDROM_REG2) == CDROM_REG3) {
					getBus("Root")->getDevice<InterruptControl>("InterruptControl")->requestInterrupt(InterruptType::CDROM, 0, 1);
				}
			}

			if (response.Number < response.NumberOfResponses) {
				command(response.CommandType, response.Number + 1);
			}

			mResponses.erase(serial);

			if (mResponses.empty()) {
				CDROM_REG0.CommandTransmissionBusy = ESX_FALSE;
			}
		}
	}

	void CDROM::decodeXAADPCMSector(const Sector& sector)
	{
		ESX_CORE_LOG_INFO("XA-ADPCM Decoding");
		
		BIT isStereo = ((sector.Subheader[3] >> 0) & 0x3) == 1 ? ESX_TRUE: ESX_FALSE;
		U32 sampleRate = ((sector.Subheader[3] >> 2) & 0x3) == 1 ? 18900 : 37800;
		U8 bitsPerSample = ((sector.Subheader[3] >> 4) & 0x3) == 1 ? 8 : 4;
		BIT emphasis = ((sector.Subheader[3] >> 6) & 0x1) == 1 ? ESX_TRUE : ESX_FALSE;

		if (bitsPerSample != 4) {
			ESX_CORE_LOG_ERROR("XA-ADPCM {} bpp not implemented yet", bitsPerSample);
			return;
		}

		if (sampleRate != 37800) {
			ESX_CORE_LOG_ERROR("XA-ADPCM {}hz sample rate not implemented yet", sampleRate);
			return;
		}

		if (emphasis == ESX_TRUE) {
			ESX_CORE_LOG_ERROR("XA-ADPCM emphasis not implemented yet");
			return;
		}

		CDROM_REG0.ADPCMFifoEmpty = ESX_FALSE;

		auto xaSector = reinterpret_cast<const XAADPCMSector*>(sector.UserData.data());

		for (const XAADPCMPortion& portion : xaSector->Portions) {
			for (I32 blk = 0; blk < 4; blk++) {
				mXAADPCMDecoder.DstLeft = 0;
				mXAADPCMDecoder.DstRight = 0;
				mXAADPCMDecoder.DstMono = 0;

				if (isStereo == ESX_TRUE) {
					decodeXAADPCM28Nibbles(portion, blk, 0, mXAADPCMDecoder.Decoded, mXAADPCMDecoder.DstLeft, mXAADPCMDecoder.OldLeft, mXAADPCMDecoder.OlderLeft, 1);
					decodeXAADPCM28Nibbles(portion, blk, 1, mXAADPCMDecoder.Decoded, mXAADPCMDecoder.DstRight, mXAADPCMDecoder.OldRight, mXAADPCMDecoder.OlderRight, 2);
				} else {
					decodeXAADPCM28Nibbles(portion, blk, 0, mXAADPCMDecoder.Decoded, mXAADPCMDecoder.DstMono, mXAADPCMDecoder.OldMono, mXAADPCMDecoder.OlderMono, 0);
					decodeXAADPCM28Nibbles(portion, blk, 1, mXAADPCMDecoder.Decoded, mXAADPCMDecoder.DstMono, mXAADPCMDecoder.OldMono, mXAADPCMDecoder.OlderMono, 0);
				}

				I32 numSamples = (isStereo == ESX_TRUE) ? mXAADPCMDecoder.DstLeft : mXAADPCMDecoder.DstMono;
				for (I32 i = 0; i < numSamples;i++) {
					Output37800Hz(mXAADPCMDecoder.Decoded[i]);
				}
			}
		}
	}

	void CDROM::decodeXAADPCM28Nibbles(const XAADPCMPortion& portion, I32 blk, U8 nibble, Array<AudioFrame, 56>& halfwords, U32& dst, I16& old, I16& older, U8 mode)
	{
		I32 shift = (portion.Headers[4 + blk * 2 + nibble] & 0x0F);
		if (shift > 12) {
			shift = 9;
		}

		U8 filter = (portion.Headers[4 + blk * 2 + nibble] & 0x30) >> 4;
		I16 f0 = pos_xa_adpcm_table[filter];
		I16 f1 = neg_xa_adpcm_table[filter];

		for (I32 i = 0; i < 28; i++) {
			U8 dataNibble = ((portion.Data[blk + i * 4] >> (nibble * 4)) & 0x0F);
			I16 t = static_cast<I16>(static_cast<U16>(dataNibble) << 12);
			I16 s = (t >> shift) + ((old * f0 + older * f1 + 32) / 64);
			s = std::clamp<I16>(s, -0x8000, 0x7FFF);

			AudioFrame& frame = halfwords[dst];
			switch (mode) {
				case 0: {
					frame.Left = frame.Right = s;
					break;
				}
				case 1: {
					frame.Left = s;
					break;
				}
				case 2: {
					frame.Right = s;
					break;
				}
			}
			dst++;

			older = old;
			old = s;
		}

	}

	void CDROM::Output44100Hz(const AudioFrame& sample)
	{
		mAudioFrames.emplace_back(sample);
	}

	void CDROM::Output37800Hz(const AudioFrame& sample)
	{
		size_t p = mRingBuf.write_index();
		mRingBuf.push_back(sample);
		mSixStep--;
		if (mSixStep == 0) {
			mSixStep = 6;

			Output44100Hz(ZigZagInterpolate(p, Table1));
			Output44100Hz(ZigZagInterpolate(p, Table2));
			Output44100Hz(ZigZagInterpolate(p, Table3));
			Output44100Hz(ZigZagInterpolate(p, Table4));
			Output44100Hz(ZigZagInterpolate(p, Table5));
			Output44100Hz(ZigZagInterpolate(p, Table6));
			Output44100Hz(ZigZagInterpolate(p, Table7));
		}
	}

	AudioFrame CDROM::ZigZagInterpolate(size_t p, const Array<I16, 29>& TableX)
	{
		I64 sumLeft = 0;
		I64 sumRight = 0;
		for (I32 i = 0; i < 29; i++) {
			const AudioFrame& frame = mRingBuf.get(p - i);
			sumLeft += (I64(frame.Left) * I64(TableX[i])) >> 15;
			sumRight += (I64(frame.Right) * I64(TableX[i])) >> 15;
		}

		I16 left = static_cast<I16>(std::clamp<I64>(sumLeft, -0x8000, 0x7FFF));
		I16 right = static_cast<I16>(std::clamp<I64>(sumRight, -0x8000, 0x7FFF));

		return AudioFrame(left, right);
	}


	void CDROM::audioVolumeApplyChanges(U8 value)
	{
		ApplyVolumeRegister reg = {};

		reg.MuteADPCM = (value >> 0) & 0x1;
		reg.ApplyChanges = (value >> 5) & 0x1;

		mAudioStreamingMuteADPCM = reg.MuteADPCM;

		if (reg.ApplyChanges) {
			mLeftCDOutToLeftSPUIn = mLeftCDOutToLeftSPUInTemp;
			mLeftCDOutToRightSPUIn = mLeftCDOutToRightSPUInTemp;
			mRightCDOutToLeftSPUIn = mRightCDOutToLeftSPUInTemp;
			mRightCDOutToRightSPUIn = mRightCDOutToRightSPUInTemp;
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
				mDataReadPointer = mLastWholeSector ? offsetof(Sector, Header) : offsetof(Sector, UserData);
				mDataWritePointer = mLastWholeSector ? CD_SECTOR_SIZE : CD_SECTOR_DATA_SIZE;
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

		if ((REG & 0xF) == 0 && !mQueuedResponses.empty()) {
			U64 serial = mQueuedResponses.front();
			mQueuedResponses.pop();
			
			BIT canDeliverResponse = ESX_TRUE;
			if (mResponses.contains(serial)) {
				Response& response = mResponses.at(serial);
				if ((response.CommandType == CommandType::ReadN || response.CommandType == CommandType::ReadS) && response.Code == INT1) {
					if ((mMode.XAFilter == ESX_TRUE && (mSectors[mCurrentSector].Subheader[0] != mXAFilterFile || (mSectors[mCurrentSector].Subheader[1] & 0x1F) != mXAFilterChannel))) {
						response.GenerateInterrupt = ESX_FALSE;
					}
				}
			}

			handleResponse(serial);
		}
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

		mResponsesSerial = 0;
		mResponse = {}; 
		mResponseSize = 0x00; 
		mResponseReadPointer = 0x00;

		mStat = {};
		mMode = {};
		mLastWholeSector = 0;

		mResponses = {};
		mQueuedResponses = {};

		mShellOpen = ESX_FALSE;
		mSeekLBA = 0x00;

		mPlayPeekRight = ESX_FALSE;
		mAudioFrames = {};
		mXAADPCMDecoder = {};
		mAudioStreamingMuteCDDA = ESX_FALSE;
		mAudioStreamingMuteADPCM = ESX_FALSE;

		//Init
		mShellOpen = ESX_FALSE;

		mStat.ShellOpen = ESX_TRUE;
		mStat.Rotating = ESX_TRUE;

		std::fill(mData.begin(), mData.end(), 0x00);

		mSetLocUnprocessed = ESX_FALSE;

		mSixStep = 6;
		mRingBuf = {};
	}

	AudioFrame CDROM::GetAudioFrame()
	{
		AudioFrame& audioFrame = mAudioFrames.front();

		I16 left = 0, right = 0;
		if (mAudioStreamingMuteCDDA ==ESX_FALSE) {
			left =  (I32(audioFrame.Left) * I32(mLeftCDOutToLeftSPUIn ) >> 7) + (I32(audioFrame.Right) * I32(mRightCDOutToLeftSPUIn ) >> 7);
			right = (I32(audioFrame.Left) * I32(mLeftCDOutToRightSPUIn) >> 7) + (I32(audioFrame.Right) * I32(mRightCDOutToRightSPUIn) >> 7);

			left = std::clamp<I16>(left, -0x8000, 0x7FFF);
			right = std::clamp<I16>(right, -0x8000, 0x7FFF);
		}
	
		mAudioFrames.pop_front();

		if (mAudioFrames.empty()) {
			CDROM_REG0.ADPCMFifoEmpty = ESX_TRUE;
		}

		return AudioFrame(left,right);
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

		if (!mMode.IgnoreBit) {
			mLastWholeSector = mMode.WholeSector;
		}
	}

	void CDROM::AbortRead()
	{
		Abort(CommandType::ReadS);
		Abort(CommandType::ReadN);
	}

	void CDROM::Abort(CommandType type)
	{
		std::erase_if(mResponses, [&](const Pair<U64, Response>& response) { return response.second.CommandType == type;  });
	}

	SubchannelQ CDROM::generateSubChannelQ()
	{
		SubchannelQ subq = {};

		U64 currentPos = mCD->getCurrentPos();

		U8 trackNumber = mCD->getTrackNumber();
		MSF trackStart = mCD->getTrackStart(trackNumber);
		U64 trackStartLBA = calculateBinaryPosition(trackStart.Minute, trackStart.Second, trackStart.Sector);
		MSF absolutePos = fromBinaryPositionToMSF(mCD->getCurrentPos());
		MSF relativePos = fromBinaryPositionToMSF(mCD->getCurrentPos() - trackStartLBA);

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