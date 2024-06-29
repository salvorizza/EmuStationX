#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {

	enum class MDECOutputDepth : U8 {
		Bit4,
		Bit8,
		Bit24,
		Bit15
	};

	struct MDECStatusRegister {
		BIT DataOutFIFOEmpty = ESX_TRUE;
		BIT DataInFIFOFull = ESX_FALSE;
		BIT CommandBusy = ESX_FALSE;
		BIT DataInRequest = ESX_FALSE;
		BIT DataOutRequest = ESX_FALSE;
		MDECOutputDepth DataOutputDepth = MDECOutputDepth::Bit4;
		BIT DataOutputSigned = ESX_FALSE;
		BIT DataOutputBit15Set = ESX_FALSE;
		U8 CurrentBlock = 0;
		U16 NumberOfParameterWords = 0xFFFF;
	};

	struct MDECControlRegister {
		BIT Reset = ESX_FALSE;
		BIT EnableDataInRequest = ESX_FALSE;
		BIT EnableDataOutRequest = ESX_FALSE;
	};

	class MDEC : public BusDevice {
	public:
		MDEC();
		~MDEC();

		virtual void clock(U64 clocks) override;

		virtual void store(const StringView& busName, U32 address, U32 value) override;
		virtual void load(const StringView& busName, U32 address, U32& output) override;

		virtual void reset() override;
	private:
		U32 getStatusRegister();
		void setStatusRegister(U32 value);

		void setControlRegister(U32 value);

		U32 getDataOrResponse();
		void setCommandOrParameters(U32 value);

	private:
		MDECStatusRegister mStatusRegister = {};
		MDECControlRegister mControlRegister = {};
	};

}