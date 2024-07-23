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

	enum class MDECCommand {
		None = 0,
		DecodeMacroblock = 1,
		SetQuantTable = 2,
		SetScaleTable = 3,
		NoFunction = 4
	};

	class MDEC : public BusDevice {
	public:
		MDEC();
		~MDEC();

		virtual void clock(U64 clocks) override;

		virtual void store(const StringView& busName, U32 address, U32 value) override;
		virtual void load(const StringView& busName, U32 address, U32& output) override;

		virtual void reset() override;

		void channelIn(U32 word);
		U32 channelOut();

	private:
		U32 getStatusRegister();
		void setStatusRegister(U32 value);

		void setControlRegister(U32 value);

		U32 getDataOrResponse();
		void setCommandOrParameters(U32 value);


		void setQuantTable();
		void setScaleTable();
		void decodeMacroblock();

		BIT rl_decode_block(Array<I16, 64>& blk, U64& src, const Array<U8, 64>& qt);
		void real_idct_core(Array<I16, 64>& blk);
		void yuv_to_rgb(const Array<I16, 64>& Crblk, const Array<I16, 64>& Cbblk, const Array<I16, 64>& Yblk, U64 xx, U64 yy);

	private:
		MDECStatusRegister mStatusRegister = {};
		MDECControlRegister mControlRegister = {};
		MDECCommand mCurrentCommand = MDECCommand::None;
		Vector<U32> mDataIn = {};
		Deque<U32> mDataOut = {};

		Array<U8, 64> mQuantTableLuminance = {};
		Array<U8, 64> mQuantTableColor = {};
		Array<I16, 64> mScaleTable = {};

		Array<U32, 256> mCurrentDecodedBlock = {};
	};

}