#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {

	using FIFO = Queue<U8>;

	enum StatusFlags {
		StatusFlagsPlay = 1 << 7,
		StatusFlagsSeek = 1 << 6,
		StatusFlagsRead = 1 << 5,
		StatusFlagsShellOpen = 1 << 4,
		StatusFlagsIdError = 1 << 3,
		StatusFlagsSeekError = 1 << 2,
		StatusFlagsRotating = 1 << 1,
		StatusFlagsError = 1 << 0,
	};

	enum class CommandType : U8 {
		None = 0x0,
		GetStat = 0x1,
		Setmode = 0xE,
		Test = 0x19,
		GetID = 0x1A,
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

	private:
		void command(CommandType command, U32 responseNumber = 1);

		void setIndexStatusRegister(U8 value);
		U8 getIndexStatusRegister();

		void setInterruptEnableRegister(U8& REG, U8 value);
		U8 getInterruptEnableRegister(U8 REG);

		void setInterruptFlagRegister(U8& REG, U8 value);
		U8 getInterruptFlagRegister(U8 REG);

		void pushParameter(U8 value);
		void flushParameters();
		U8 popParameter();

		void pushResponse(U8 value);
		U8 popResponse();

		void pushData(U8 value);
		U8 popData();

	private:
		IndexStatusRegister CDROM_REG0;
		U8 CDROM_REG1 = 0x00;
		U8 CDROM_REG2 = 0x00;
		U8 CDROM_REG3 = 0x00;

		Array<U8, 16> mParameters; U8 mParametersSize = 0x00, mParametersReadPointer = 0x00;

		Array<U8, 16> mResponse; U8 mResponseSize = 0x00, mResponseReadPointer = 0x00;

		FIFO mData;

		StatusFlags mStat = StatusFlagsRotating;

		Queue<Response> mResponses;
	};

}