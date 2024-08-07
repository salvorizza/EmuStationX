#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"
#include "SerialDevice.h"

namespace esx {

	struct TransmissionFIFO {
		Deque<U8> Data;

		BIT Push(U8 value) {
			if (Data.size() == Size()) {
				Data.back() = value;
				return ESX_FALSE;
			}
			Data.push_back(value);
			return ESX_TRUE;
		}

		U8 Pop() {
			ESX_ASSERT(Data.size() != 0, "Empty");
			U8& data = Data.front();
			Data.pop_front();
			return data;
		}

		bool Empty() {
			return Data.empty();
		}

		void Clear() {
			Data.clear();
		}

		static size_t Size() {
			return 8;
		}
	};


	enum class BaudrateReloadFactor : U8 {
		SIO1_STOP_SIO0_MUL1 = 0,
		MUL1 = 1,
		MUL16 = 2,
		MUL64 = 3
	};

	enum class CharacterLength : U8 {
		Bits5 = 0,
		Bits6 = 1,
		Bits7 = 2,
		Bits8 = 3
	};

	enum class ParityType : U8 {
		Even = 0,
		Odd = 1
	};

	enum class StopBitLength : U8 {
		Reserved = 0,
		Bits1 = 1,
		Bits1_5 = 2,
		Bits2 = 3
	};

	enum class ClockPolarity : U8 {
		HighWhenIdle = 0,
		LowWhenIdle = 1
	};

	enum class InterruptMode : U8 {
		Bytes1 = 0,
		Bytes2 = 1,
		Bytes4 = 2,
		Bytes8 = 3
	};

	struct StatRegister {
		BIT TXFifoNotFull = ESX_TRUE;
		BIT RXFifoNotEmpty = ESX_FALSE;
		BIT TXIdle = ESX_TRUE;
		BIT RXParityError = ESX_FALSE;

		BIT RXFifoOverrun = ESX_FALSE;//SIO1
		BIT RXBadStopBit = ESX_FALSE;//SIO1
		BIT RXInputLevel = ESX_FALSE;//SIO1

		BIT DSRInputLevel = ESX_FALSE;

		BIT CTSInputLevel = ESX_FALSE;//SIO1

		BIT InterruptRequest = ESX_FALSE;
		BIT Unknown = ESX_FALSE;

		U32 BaudrateTimer = 0;
	};

	struct ModeRegister {
		BaudrateReloadFactor BaudrateReloadFactor = BaudrateReloadFactor::SIO1_STOP_SIO0_MUL1;
		CharacterLength CharacterLength = CharacterLength::Bits5;
		BIT ParityEnable = ESX_FALSE;
		ParityType ParityType = ParityType::Even;
		StopBitLength StopBitLength = StopBitLength::Reserved;//SIO1
		ClockPolarity ClockPolarity = ClockPolarity::HighWhenIdle;//SIO0
	};

	struct SIOControlRegister {
		BIT TXEnable = ESX_FALSE;
		BIT DTROutputLevel = ESX_FALSE;
		BIT RXEnable = ESX_FALSE;
		BIT TXOutputLevel = ESX_FALSE;
		BIT Acknowledge = ESX_FALSE;
		BIT RTSOutputLevel = ESX_FALSE;
		BIT Reset = ESX_FALSE;
		BIT Unknown = ESX_FALSE;//SIO1
		InterruptMode InterruptMode = InterruptMode::Bytes1;
		BIT TXInterruptEnable = ESX_FALSE;
		BIT RXInterruptEnable = ESX_FALSE;
		BIT DSRInterruptEnable = ESX_FALSE;
		U8 PortSelect = 0;
	};

	struct MiscRegister {
		U16 InternalRegister = 0;//SIO1
	};

	struct BaudRegister {
		U16 ReloadValue = 0;
	};

	enum SerialPort {
		Port1,
		Port2
	};

	class Controller;

	class SIO : public BusDevice {
	public:
		SIO(U8 id);
		~SIO();

		virtual void clock(U64 clocks) override;

		void fallingEdge();
		void risingEdge();

		void dsr(U64 delay = 0);

		virtual void load(const StringView& busName, U32 address, U32& output) override;

		virtual void store(const StringView& busName, U32 address, U16 value) override;
		virtual void load(const StringView& busName, U32 address, U16& output) override;

		virtual void store(const StringView& busName, U32 address, U8 value) override;
		virtual void load(const StringView& busName, U32 address, U8& output) override;

		inline void plugDevice(SerialPort port, const SharedPtr<SerialDevice>& device) { mPorts[port][(U8)device->getDeviceType()] = device; }

		virtual void reset();
	private:
		void setDataRegister(U32 value);
		U32 getDataRegister(U8 dataAccess);

		U32 getStatRegister();

		void setModeRegister(U16 value);
		U16 getModeRegister();

		void setControlRegister(U16 value);
		U16 getControlRegister();

		void setMiscRegister(U16 value);
		U16 getMiscRegister();

		void setBaudRegister(U16 value);
		U16 getBaudRegister();

		void reloadBaudTimer();

		BIT canTransferStart();
		BIT canReceiveData();

	private:
		U8 mID;

		U8 mTX = 0xFF;
		TransmissionFIFO mRX;
		StatRegister mStatRegister;
		ModeRegister mModeRegister;
		SIOControlRegister mControlRegister;
		MiscRegister mMiscRegister;
		BaudRegister mBaudRegister;

		BIT mSerialClock = ESX_FALSE;

		BIT mLatchedTXEN = ESX_FALSE;
		BIT mRead = ESX_FALSE;

		Array<Array<SharedPtr<SerialDevice>, 2>, 2> mPorts;

		ShiftRegister mTXShift, mRXShift;
	};

}