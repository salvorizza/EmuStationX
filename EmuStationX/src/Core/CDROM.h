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
		GetStat = 0x1,
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

	class CDROM : public BusDevice {
	public:
		CDROM();
		~CDROM();

		virtual void store(const StringView& busName, U32 address, U8 value) override;
		virtual void load(const StringView& busName, U32 address, U8& output) override;

	private:
		void command(CommandType command);

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

	};

}