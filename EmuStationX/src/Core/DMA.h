#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"

#include "Core/RAM.h"
#include "Core/GPU.h"

namespace esx {

	enum class Port : U8 {
		MDECin,
		MDECout,
		GPU,
		CDROM,
		SPU,
		PIO,
		OTC
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
		Manual = 0,
		Blocks = 1,
		LinkedList = 2,
		Reserved = 3
	};

	struct Channel {
		BIT Enable = ESX_FALSE;
		Direction Direction = Direction::ToMainRAM;
		Step Step = Step::Forward;
		BIT ChoppingEnable = ESX_FALSE;
		SyncMode SyncMode = SyncMode::Manual;
		U8 ChoppingDMAWindowSize = 0;
		U8 ChoppingCPUWindowSize = 0;
		BIT Trigger = ESX_FALSE;
		U8 Dummy = 0;

		U32 BaseAddress = 0;

		U16 BlockSize;
		U16 BlockCount;
	};

	struct ControlRegister {
		U8 MDECinPriority = 0;
		BIT MDECinMasterEnable = ESX_FALSE;
		U8 MDECoutPriority = 0;
		BIT MDECoutMasterEnable = ESX_FALSE;
		U8 GPUPriority = 0;
		BIT GPUMasterEnable = ESX_FALSE;
		U8 CDROMPriority = 0;
		BIT CDROMMasterEnable = ESX_FALSE;
		U8 SPUPriority = 0;
		BIT SPUMasterEnable = ESX_FALSE;
		U8 PIOPriority = 0;
		BIT PIOMasterEnable = ESX_FALSE;
		U8 OTCPriority = 0;
		BIT OTCMasterEnable = ESX_FALSE;
		U8 Dummy1 = 0;
		U8 Dummy2 = 0;
	};

	struct InterruptRegister {
		U8 Dummy = 0;
		BIT ForceIRQ = ESX_FALSE;
		U8 IRQEnable = 0;
		BIT IRQMasterEnable = ESX_FALSE;
		U8 IRQFlags = 0;

		BIT IRQMasterFlag() {
			return (ForceIRQ || (IRQMasterEnable && ((IRQEnable & IRQFlags) > 0))) ? ESX_TRUE : ESX_FALSE;
		}
	};

	class DMA : public BusDevice {
	public:
		DMA();
		~DMA();
		
		virtual void store(const StringView& busName, U32 address, U32 value) override;
		virtual void load(const StringView& busName, U32 address, U32& output) override;

	private:
		void setChannelControl(Port port, U32 channelControl);
		U32 getChannelControl(Port port);

		void setChannelBaseAddress(Port port, U32 value);
		U32 getChannelBaseAddress(Port port);

		void setChannelBlockControl(Port port, U32 value);
		U32 getChannelBlockControl(Port port);

		BIT isChannelActive(Port port);
		void setChannelDone(Port port);

		void setInterruptRegister(U32 value);
		U32 getInterruptRegister();

		void setControlRegister(U32 value);
		U32 getControlRegister();

		void startTransfer(Port port);

		void startBlockTransfer(Port port, const Channel& channel);
		void startLinkedListTransfer(Port port, const Channel& channel);

	private:
		ControlRegister mControlRegister;
		InterruptRegister mInterruptRegister;
		std::array<Channel, 7> mChannels;
	};

}