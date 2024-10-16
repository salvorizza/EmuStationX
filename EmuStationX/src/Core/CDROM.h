#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"

#include "CD/CompactDisk.h"

namespace esx {

	constexpr size_t CD_READ_DELAY = 33868800llu * 0x930llu / 4llu / 44100llu;
	constexpr size_t CD_READ_DELAY_2X = CD_READ_DELAY / 2llu;
	constexpr size_t CD_1_MS = 33868800llu / 1000llu;

	using FIFO = Queue<U8>;

	enum GetIdFlags {
		GetIdFlagsUnlicensed = 1 << 7,
		GetIdFlagsMissing = 1 << 6,
		GetIdFlagsAudioCD = 1 << 4
	};

	enum GetIdDiskType {
		GetIdDiskTypeMode1 = 0x00,
		GetIdDiskTypeMode2 = 0x20
	};

	struct CDROMStatusRegister {
		BIT Play = ESX_FALSE;
		BIT Seek = ESX_FALSE;
		BIT Read = ESX_FALSE;
		BIT ShellOpen = ESX_FALSE;
		BIT IdError = ESX_FALSE;
		BIT SeekError = ESX_FALSE;
		BIT Rotating = ESX_FALSE;
		BIT Error = ESX_FALSE;
	};

	struct CDROMModeRegister {
		BIT DoubleSpeed = ESX_FALSE;
		BIT XAADPCM = ESX_FALSE;
		BIT WholeSector = ESX_FALSE;
		BIT IgnoreBit = ESX_FALSE;
		BIT XAFilter = ESX_FALSE;
		BIT Report = ESX_FALSE;
		BIT AutoPause = ESX_FALSE;
		BIT CDDA = ESX_FALSE;
	};

	enum class CommandType : U8 {
		None = 0x00,
		GetStat = 0x01,
		Setloc = 0x02,
		Play = 0x03,
		ReadN = 0x06,
		Stop = 0x08,
		Pause = 0x09,
		Init = 0x0A,
		Mute = 0x0B,
		Demute = 0x0C,
		Setfilter = 0x0D,
		Setmode = 0x0E,
		GetlocL = 0x10,
		GetlocP = 0x11,
		GetTN = 0x13,
		GetTD = 0x14,
		SeekL = 0x15,
		Test = 0x19,
		GetID = 0x1A,
		ReadS = 0x1B,
		ReadTOC = 0x1E
	};

	struct IndexStatusRegister {
		U8 Index = 0;
		BIT ADPCMFifoEmpty = ESX_TRUE;
		BIT ParameterFifoEmpty = ESX_TRUE;
		BIT ParameterFifoFull = ESX_FALSE;
		BIT ResponseFifoEmpty = ESX_TRUE;
		BIT DataFifoEmpty = ESX_TRUE;
		BIT CommandTransmissionBusy = ESX_FALSE;
	};

	struct RequestRegister {
		BIT WantCommandStartInterrupt = ESX_FALSE;
		BIT BFWR = ESX_FALSE;;
		BIT WantData = ESX_FALSE;
	};

	struct ApplyVolumeRegister {
		BIT MuteADPCM = ESX_FALSE;
		BIT ApplyChanges = ESX_FALSE;
	};

	struct XAADPCMPortion {
		Array<U8, 16> Headers = {};
		Array<U8, 4*28> Data = {};
	};

	struct XAADPCMSector {
		Array<XAADPCMPortion, 0x12> Portions = {};
	};

	struct XAADPCMDecoder {
		I16 OldLeft = 0;
		I16 OlderLeft = 0;

		I16 OldRight = 0;
		I16 OlderRight = 0;

		I16 OldMono = 0;
		I16 OlderMono = 0;

		Array<AudioFrame, 56> Decoded = {};
		U32 DstLeft = 0;
		U32 DstRight = 0;
		U32 DstMono = 0;
	};

	const Array<I16,29> Table1 = {
		0x0,0x0,0x0,0x0,
		0x0,-0x0002,+0x000A,-0x0022,
		+0x0041,-0x0054,+0x0034,+0x0009,
		-0x010A,+0x0400,-0x0A78,+0x234C,
		+0x6794,-0x1780,+0x0BCD,-0x0623,
		+0x0350,-0x016D,+0x006B,+0x000A,
		-0x0010,+0x0011,-0x0008,+0x0003,
		-0x0001
	};

	const Array<I16, 29> Table2 = {
		0x0,
		0x0,
		0x0,
		-0x0002,
		0x0,
		+0x0003,
		-0x0013,
		+0x003C,
		-0x004B,
		+0x00A2,
		-0x00E3,
		+0x0132,
		-0x0043,
		-0x0267,
		+0x0C9D,
		+0x74BB,
		-0x11B4,
		+0x09B8,
		-0x05BF,
		+0x0372,
		-0x01A8,
		+0x00A6,
		-0x001B,
		+0x0005,
		+0x0006,
		-0x0008,
		+0x0003,
		-0x0001,
		0x0
	};

	const Array<I16, 29> Table3 = {
		0x0,
		0x0,
		-0x0001,
		+0x0003,
		-0x0002,
		-0x0005,
		+0x001F,
		-0x004A,
		+0x00B3,
		-0x0192,
		+0x02B1,
		-0x039E,
		+0x04F8,
		-0x05A6,
		+0x7939,
		-0x05A6,
		+0x04F8,
		-0x039E,
		+0x02B1,
		-0x0192,
		+0x00B3,
		-0x004A,
		+0x001F,
		-0x0005,
		-0x0002,
		+0x0003,
		-0x0001,
		0x0,
		0x0
	};

	const Array<I16, 29> Table4 = {
		0x0,
		-0x0001,
		+0x0003,
		-0x0008,
		+0x0006,
		+0x0005,
		-0x001B,
		+0x00A6,
		-0x01A8,
		+0x0372,
		-0x05BF,
		+0x09B8,
		-0x11B4,
		+0x74BB,
		+0x0C9D,
		-0x0267,
		-0x0043,
		+0x0132,
		-0x00E3,
		+0x00A2,
		-0x004B,
		+0x003C,
		-0x0013,
		+0x0003,
		0x0,
		-0x0002,
		0x0,
		0x0,
		0x0
	};

	const Array<I16, 29> Table5 = {
		-0x0001,
		+0x0003,
		-0x0008,
		+0x0011,
		-0x0010,
		+0x000A,
		+0x006B,
		-0x016D,
		+0x0350,
		-0x0623,
		+0x0BCD,
		-0x1780,
		+0x6794,
		+0x234C,
		-0x0A78,
		+0x0400,
		-0x010A,
		+0x0009,
		+0x0034,
		-0x0054,
		+0x0041,
		-0x0022,
		+0x000A,
		-0x0001,
		0x0,
		+0x0001,
		0x0,
		0x0,
		0x0
	};

	const Array<I16, 29> Table6 = {
		+0x0002,
		-0x0008,
		+0x0010,
		-0x0023,
		+0x002B,
		+0x001A,
		-0x00EB,
		+0x027B,
		-0x0548,
		+0x0AFA,
		-0x16FA,
		+0x53E0,
		+0x3C07,
		-0x1249,
		+0x080E,
		-0x0347,
		+0x015B,
		-0x0044,
		-0x0017,
		+0x0046,
		-0x0023,
		+0x0011,
		-0x0005,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0
	};

	const Array<I16, 29> Table7 = {
		-0x0005,
		+0x0011,
		-0x0023,
		+0x0046,
		-0x0017,
		-0x0044,
		+0x015B,
		-0x0347,
		+0x080E,
		-0x1249,
		+0x3C07,
		+0x53E0,
		-0x16FA,
		+0x0AFA,
		-0x0548,
		+0x027B,
		-0x00EB,
		+0x001A,
		+0x002B,
		-0x0023,
		+0x0010,
		-0x0008,
		+0x0002,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0,
		0x0
	};

	enum ResponseCode : U8 {
		INT0  = 0b00000,
		INT1  = 0b00001,
		INT2  = 0b00010,
		INT3  = 0b00011,
		INT4  = 0b00100,
		INT5  = 0b00101,
		INT6  = 0b00110,
		INT7  = 0b00111,
		INT8  = 0b01000,
		INT10 = 0b10000
	};

	struct Response {
		Array<U8, 16> Data; 
		U8 Size = 0x00, ReadPointer = 0x00;
		ResponseCode Code = ResponseCode::INT0;
		U64 TargetCycle = 0;
		U32 Number = 0;
		U32 NumberOfResponses = 0;
		CommandType CommandType = CommandType::None;
		BIT GenerateInterrupt = ESX_TRUE;

		void Push(U8 value) { Data[Size++] = value; }
		U8 Pop() { return Data[ReadPointer++]; }
		void Clear() { ReadPointer = 0; Size = 0; }
		BIT Empty() { return Size == ReadPointer; }
	};

	class CDROM : public BusDevice {
	public:
		CDROM();
		~CDROM();

		virtual void clock(U64 clocks)override;

		virtual void store(const StringView& busName, U32 address, U8 value) override;
		virtual void load(const StringView& busName, U32 address, U8& output) override;

		void insertCD(const SharedPtr<CompactDisk>& cd) { mCD = cd; }

		U8 popData();

		virtual void reset() override;

		BIT HasAudioFramesAvailable() { return mAudioFrames.empty() ? ESX_FALSE : ESX_TRUE; }
		AudioFrame GetAudioFrame();

	private:
		void command(CommandType command, U32 responseNumber = 1);
		void handleResponse(U64 serial);

		void decodeXAADPCMSector(const Sector& sector);
		void decodeXAADPCM28Nibbles(const XAADPCMPortion& portion, I32 blk, U8 nibble, Array<AudioFrame,56>& halfwords, U32& dst, I16& old, I16& older, U8 mode);

		void Output44100Hz(const AudioFrame& sample);
		void Output37800Hz(const AudioFrame& sample);
		AudioFrame ZigZagInterpolate(size_t p, const Array<I16,29>& TableX);

		void audioVolumeApplyChanges(U8 value);

		void setIndexStatusRegister(U8 value);
		U8 getIndexStatusRegister();

		void setRequestRegister(U8 value);

		void setInterruptEnableRegister(U8& REG, U8 value);
		U8 getInterruptEnableRegister(U8 REG);

		void setInterruptFlagRegister(U8& REG, U8 value);
		U8 getInterruptFlagRegister(U8 REG);

		void pushParameter(U8 value);
		void flushParameters();
		U8 popParameter();

		void pushResponse(U8 value);
		U8 popResponse();


		U8 getStatus();

		U8 getMode();
		void setMode(U8 value);

		void AbortRead();
		void Abort(CommandType type);
		SubchannelQ generateSubChannelQ();


	private:
		IndexStatusRegister CDROM_REG0;
		U8 CDROM_REG1 = 0x00;
		U8 CDROM_REG2 = 0x00;
		U8 CDROM_REG3 = 0x00;

		U8 mLeftCDOutToLeftSPUInTemp = 0;
		U8 mLeftCDOutToRightSPUInTemp = 0;
		U8 mRightCDOutToRightSPUInTemp = 0;
		U8 mRightCDOutToLeftSPUInTemp = 0;

		U8 mLeftCDOutToLeftSPUIn = 0;
		U8 mLeftCDOutToRightSPUIn = 0;
		U8 mRightCDOutToRightSPUIn = 0;
		U8 mRightCDOutToLeftSPUIn = 0;

		Queue<U8> mParameters;

		Array<U8, 16> mResponse; U8 mResponseSize = 0x00, mResponseReadPointer = 0x00;

		Array<U8, CD_SECTOR_SIZE> mData; U32 mDataReadPointer = 0; U32 mDataWritePointer = 0;
		Array<Sector, 8> mSectors; U32 mOldSector = 0; U32 mCurrentSector = 0; U32 mNextSector = 0;

		CDROMStatusRegister mStat = {};
		CDROMModeRegister mMode = {};
		U8 mLastWholeSector = 0;

		U64 mResponsesSerial = 0;
		UnorderedMap<U64, Response> mResponses;
		Queue<U64> mQueuedResponses;

		BIT mShellOpen = ESX_FALSE;
		SharedPtr<CompactDisk> mCD;
		U64 mSeekLBA;
		BIT mSetLocUnprocessed = ESX_FALSE;
		
		BIT mPlayPeekRight = ESX_FALSE;
		BIT mAudioStreamingMuteCDDA = ESX_FALSE;
		BIT mAudioStreamingMuteADPCM = ESX_FALSE;

		U8 mXAFilterFile = 0;
		U8 mXAFilterChannel = 0;


		SubchannelQ mLastSubQ = {};
		Deque<AudioFrame> mAudioFrames = {};
		XAADPCMDecoder mXAADPCMDecoder = {};

		I16 mSixStep = 6;
		CircularBuffer<AudioFrame, 32> mRingBuf = {};
	};

}