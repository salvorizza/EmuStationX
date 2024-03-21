#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {

	using FIFO = Queue<U8>;

	enum CommandType : U8 {
		Test = 0x19
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
		U8 popParameter();

		void pushResponse(U8 value);
		U8 popResponse();

		void pushData(U8 value);
		U8 popData();

	private:
		IndexStatusRegister CDROM_REG0;
		U8 CDROM_REG1;
		U8 CDROM_REG2;
		U8 CDROM_REG3;

		FIFO mParameters;
		FIFO mResponse;
		FIFO mData;

	};

}