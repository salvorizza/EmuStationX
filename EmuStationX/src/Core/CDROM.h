#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"

#include "CD/CompactDisk.h"

namespace esx {

	constexpr size_t CD_READ_DELAY = 33868800 / CD_SECTORS_PER_SECOND;
	constexpr size_t CD_READ_DELAY_2X = 33868800 / (2 * CD_SECTORS_PER_SECOND);

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
		ReadN = 0x06,
		Pause = 0x09,
		Init = 0x0A,
		Mute = 0x0B,
		Demute = 0x0C,
		Setmode = 0x0E,
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

		constexpr static U8 fromBCD(U8 bcd) { return ((bcd >> 4) & 0xF) * 10 + ((bcd >> 0) & 0xF); }
	private:
		void command(CommandType command, U32 responseNumber = 1);

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


	private:
		IndexStatusRegister CDROM_REG0;
		U8 CDROM_REG1 = 0x00;
		U8 CDROM_REG2 = 0x00;
		U8 CDROM_REG3 = 0x00;

		U8 mLeftCDOutToLeftSPUIn = 0;
		U8 mLeftCDOutToRightSPUIn = 0;
		U8 mRightCDOutToRightSPUIn = 0;
		U8 mRightCDOutToLeftSPUIn = 0;

		Queue<U8> mParameters;

		Array<U8, 16> mResponse; U8 mResponseSize = 0x00, mResponseReadPointer = 0x00;

		Deque<U8> mData;
		Deque<Sector> mSectors;

		CDROMStatusRegister mStat = {};
		CDROMModeRegister mMode = {};

		Queue<Response> mResponses;

		BIT mShellOpen = ESX_FALSE;
		SharedPtr<CompactDisk> mCD;
		U64 mSeekLBA;
		BIT mSetLocUnprocessed = ESX_FALSE;
	};

}