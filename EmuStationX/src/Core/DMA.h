#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {

	enum class Port : U8 {
		MDECin,
		MDECout,
		GPU,
		CDROM,
		SPU,
		PIO,
		OTC,
		Max
	};

	enum class Direction : U8 {
		ToMainRAM = 0,
		FromMainRAM = 1
	};

	enum class Step : U8 {
		Forward = 0,
		Backward = 1
	};

	enum class SyncMode : U8 {
		Burst = 0,
		Slice = 1,
		LinkedList = 2,
		Reserved = 3
	};


	struct TransferStatus {
		U32 BlockRemainingSize;
		U32 BlockCurrentAddress;

		U32 LinkedListCurrentNodeAddress;
		U32 LinkedListCurrentNodeHeader;
		U32 LinkedListNextNodeAddress;
		U32 LinkedListRemainingSize;
		U32 LinkedListPacketAddress;
	};

	struct Channel {
		Port Port = Port::Max;

		BIT TransferStartOrBusy = ESX_FALSE;
		Direction Direction = Direction::ToMainRAM;
		Step Step = Step::Forward;
		BIT ChoppingEnable = ESX_FALSE;
		SyncMode SyncMode = SyncMode::Burst;
		U8 ChoppingDMAWindowSize = 0;
		U8 ChoppingCPUWindowSize = 0;
		BIT ForceTransferStart = ESX_FALSE;
		U8 Dummy = 0;

		U8 Priority = 0;
		BIT MasterEnable = ESX_FALSE;

		U32 BaseAddress = 0;

		U16 BlockSize = 0;
		U16 BlockCount = 0;

		TransferStatus TransferStatus;
	};


	struct ControlRegister {
		U8 Dummy1 = 0;
		U8 Dummy2 = 0;
	};

	struct InterruptRegister {
		U8 IRQCompletionInterrupt = 0;
		U8 Unused = 0;
		BIT BusErrorFlag = ESX_FALSE;
		U8 IRQEnable = 0;
		BIT IRQMasterEnable = ESX_FALSE;
		U8 IRQFlags = 0;

		BIT IRQMasterFlag() {
			return (BusErrorFlag || (IRQMasterEnable && IRQFlags > 0)) ? ESX_TRUE : ESX_FALSE;
		}
	};

	constexpr Array<U64, 7> CLOCKS_PER_WORD = {
		1,
		1,
		1,
		40,
		4,
		20,
		1
	};

	class InterruptControl;
	class GPU;
	class CDROM;
	class SPU;
	class RAM;
	class MDEC;
	class R3000;

	class DMA : public BusDevice {
	public:
		DMA();
		~DMA();

		virtual void clock(U64 clocks) override;
		
		virtual void store(const StringView& busName, U32 address, U32 value) override;
		virtual void load(const StringView& busName, U32 address, U32& output) override;

		virtual void store(const StringView& busName, U32 address, U8 value) override;
		virtual void load(const StringView& busName, U32 address, U8& output) override;

		inline BIT isRunning() const { return mRunningDMAs != 0; }

		virtual void reset() override;
		virtual void init() override;
	private:
		void setChannelControl(Port port, U32 channelControl);
		U32 getChannelControl(Port port);

		void setChannelBaseAddress(Port port, U32 value);
		U32 getChannelBaseAddress(Port port);

		void setChannelBlockControl(Port port, U32 value);
		U32 getChannelBlockControl(Port port);

		BIT isChannelActive(Port port);
		void setChannelDone(Channel& channel);

		void setInterruptRegister(U32 value);
		U32 getInterruptRegister();

		void setControlRegister(U32 value);
		U32 getControlRegister();

		void startTransfer(Port port);

		void startBlockTransfer(Channel& channel);
		BIT clockBlockTransfer(Channel& channel);

		void startLinkedListTransfer(Channel& channel);
		BIT clockLinkedListTransfer(Channel& channel);

	private:
		ControlRegister mControlRegister;
		InterruptRegister mInterruptRegister;
		Array<Channel, 7> mChannels;
		Vector<Port> mPriorityPorts = {
			Port::MDECin,
			Port::MDECout,
			Port::GPU,
			Port::CDROM,
			Port::SPU,
			Port::PIO,
			Port::OTC
		};
		U8 mRunningDMAs = 0;

		SharedPtr<InterruptControl> mInterruptControl;
		SharedPtr<Bus> mBus;
		SharedPtr<GPU> mGPU;
		SharedPtr<CDROM> mCDROM;
		SharedPtr<DMA> mDMA;
		SharedPtr<SPU> mSPU;
		SharedPtr<MDEC> mMDEC;
		SharedPtr<RAM> mRAM;
		SharedPtr<R3000> mCPU;
	};

}