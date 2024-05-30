#include "DMA.h"

#include "InterruptControl.h"

namespace esx {

	DMA::DMA()
		: BusDevice(ESX_TEXT("DMA"))
	{
		addRange(ESX_TEXT("Root"), 0x1F801080, BYTE(0x75), 0xFFFFFFFF);

		for (U8 port = 0; port < (U8)Port::Max; port++) {
			mChannels[port].Port = (Port)port;
		}

		setControlRegister(0x07654321);
		setInterruptRegister(0);
	}

	DMA::~DMA()
	{
	}

	void DMA::clock(U64 clocks)
	{
		for (Port& port : mPriorityPorts) {
			Channel& channel = mChannels[(U8)port];

			if (channel.MasterEnable && channel.Enable) {
				switch (channel.SyncMode)
				{
					case SyncMode::LinkedList:
						clockLinkedListTransfer(channel);
						break;

					default:
						clockBlockTransfer(channel);
						break;
				}
				break;
			}
		}
	}

	void DMA::store(const StringView& busName, U32 address, U32 value)
	{
		switch (address & (~0xF)) {
			case 0x1F8010F0: {
				if (address == 0x1F8010F0) {
					setControlRegister(value);
				} else {
					setInterruptRegister(value);
				}
				break;
			}
			default: {
				Port port = (Port)(((address >> 4) & 0xF) - 0x8);

				switch (address & 0xF) {
					case 0x0: {
						setChannelBaseAddress(port, value);
						break;
					}
					case 0x4: {
						setChannelBlockControl(port, value);
						break;
					}
					case 0x8: {
						setChannelControl(port, value);
						break;
					}
				}

				if (isChannelActive(port)) {
					startTransfer(port);
				}
			}
		}
	}

	void DMA::load(const StringView& busName, U32 address, U32& output)
	{

		switch (address & (~0xF)) {
			case 0x1F8010F0: {
				if (address == 0x1F8010F0) {
					output = getControlRegister();
				}
				else {
					output = getInterruptRegister();
				}
				break;
			}
			default: {
				Port channel = (Port)(((address >> 4) & 0xF) - 0x8);

				switch (address & 0xF) {
					case 0x0: {
						output = getChannelBaseAddress(channel);
						break;
					}
					case 0x4: {
						output = getChannelBlockControl(channel);
						break;
					}
					case 0x8: {
						output = getChannelControl(channel);
						break;
					}
				}
			}
		}

	}

	void DMA::setChannelControl(Port port, U32 channelControl)
	{
		mChannels[(U8)port].Direction = (Direction)((channelControl >> 0) & 0x1);
		mChannels[(U8)port].Step = (Step)((channelControl >> 1) & 0x1);
		mChannels[(U8)port].ChoppingEnable = ((channelControl >> 8) & 0x1);
		mChannels[(U8)port].SyncMode = (SyncMode)((channelControl >> 9) & 0x3);
		mChannels[(U8)port].ChoppingDMAWindowSize = ((channelControl >> 16) & 0x7);
		mChannels[(U8)port].ChoppingCPUWindowSize = ((channelControl >> 20) & 0x7);
		mChannels[(U8)port].Enable = ((channelControl >> 24) & 0x1);
		mChannels[(U8)port].Trigger = ((channelControl >> 28) & 0x1);
		mChannels[(U8)port].Dummy = ((channelControl >> 29) & 0x3);
	}

	U32 DMA::getChannelControl(Port port)
	{
		U32 result = 0;

		result |= ((U8)mChannels[(U8)port].Direction) << 0;
		result |= ((U8)mChannels[(U8)port].Step) << 1;
		result |= ((U8)mChannels[(U8)port].ChoppingEnable) << 8;
		result |= ((U8)mChannels[(U8)port].SyncMode) << 9;
		result |= ((U8)mChannels[(U8)port].ChoppingDMAWindowSize) << 16;
		result |= ((U8)mChannels[(U8)port].ChoppingCPUWindowSize) << 20;
		result |= ((U8)mChannels[(U8)port].Enable) << 24;
		result |= ((U8)mChannels[(U8)port].Trigger) << 28;
		result |= ((U8)mChannels[(U8)port].Dummy) << 29;

		return result;
	}

	void DMA::setChannelBaseAddress(Port port, U32 value)
	{
		mChannels[(U8)port].BaseAddress = value & 0xFFFFFF;
	}

	U32 DMA::getChannelBaseAddress(Port port)
	{
		return mChannels[(U8)port].BaseAddress;
	}

	void DMA::setChannelBlockControl(Port port, U32 value)
	{
		mChannels[(U8)port].BlockSize = value & 0xFFFF;
		mChannels[(U8)port].BlockCount = value >> 16;
	}

	U32 DMA::getChannelBlockControl(Port port)
	{
		U32 result = 0;

		result |= (mChannels[(U8)port].BlockSize) << 0;
		result |= (mChannels[(U8)port].BlockCount) << 16;

		return result;
	}

	BIT DMA::isChannelActive(Port port)
	{
		Channel& channel = mChannels[(U8)port];

		BIT trigger = ESX_TRUE;
		if (channel.SyncMode == SyncMode::Manual) {
			trigger = channel.Trigger;
		}

		return (channel.MasterEnable && channel.Enable && trigger) ? ESX_TRUE : ESX_FALSE;
	}

	void DMA::setChannelDone(Channel& channel)
	{
		channel.Enable = ESX_FALSE;

		U8 flag = 1 << (U8)channel.Port;

		if (mInterruptRegister.IRQEnable & flag) {
			BIT oldIRQ = mInterruptRegister.IRQMasterFlag();
			mInterruptRegister.IRQFlags |= flag;
			BIT newIRQ = mInterruptRegister.IRQMasterFlag();

			getBus("Root")->getDevice<InterruptControl>("InterruptControl")->requestInterrupt(InterruptType::DMA, oldIRQ, newIRQ);
		}
		mRunningDMAs--;
	}

	void DMA::setInterruptRegister(U32 value)
	{
		mInterruptRegister.Dummy = (value >> 0) & 0x1F;
		mInterruptRegister.ForceIRQ = (value >> 15) & 0x1;
		mInterruptRegister.IRQEnable = (value >> 16) & 0x7F;
		mInterruptRegister.IRQMasterEnable = (value >> 23) & 0x1;
		mInterruptRegister.IRQFlags &= ~((value >> 24) & 0x7F);
	}

	U32 DMA::getInterruptRegister()
	{
		U32 result = 0;

		result |= mInterruptRegister.Dummy << 0;
		result |= mInterruptRegister.ForceIRQ << 15;
		result |= mInterruptRegister.IRQEnable << 16;
		result |= mInterruptRegister.IRQMasterEnable << 23;
		result |= mInterruptRegister.IRQFlags << 24;
		result |= mInterruptRegister.IRQMasterFlag() << 31;

		return result;
	}

	void DMA::setControlRegister(U32 value)
	{
		mChannels[(U8)Port::MDECin].Priority = (value >> 0) & 0x3;
		mChannels[(U8)Port::MDECin].MasterEnable = (value >> 3) & 0x1;
		mChannels[(U8)Port::MDECout].Priority = (value >> 4) & 0x3;
		mChannels[(U8)Port::MDECout].MasterEnable = (value >> 7) & 0x1;
		mChannels[(U8)Port::GPU].Priority = (value >> 8) & 0x3;
		mChannels[(U8)Port::GPU].MasterEnable = (value >> 11) & 0x1;
		mChannels[(U8)Port::CDROM].Priority = (value >> 12) & 0x3;
		mChannels[(U8)Port::CDROM].MasterEnable = (value >> 15) & 0x1;
		mChannels[(U8)Port::SPU].Priority = (value >> 16) & 0x3;
		mChannels[(U8)Port::SPU].MasterEnable = (value >> 19) & 0x1;
		mChannels[(U8)Port::PIO].Priority = (value >> 20) & 0x3;
		mChannels[(U8)Port::PIO].MasterEnable = (value >> 23) & 0x1;
		mChannels[(U8)Port::OTC].Priority = (value >> 24) & 0x3;
		mChannels[(U8)Port::OTC].MasterEnable = (value >> 27) & 0x1;
		mControlRegister.Dummy1 = (value >> 28) & 0x3;
		mControlRegister.Dummy2 = (value >> 31) & 0x1;

		mPriorityPorts = {
			Port::MDECin,
			Port::MDECout,
			Port::GPU,
			Port::CDROM,
			Port::SPU,
			Port::PIO,
			Port::OTC
		};

		std::sort(mPriorityPorts.begin(), mPriorityPorts.end(), [&](Port& a, Port& b) { return mChannels[(U8)a].Priority < mChannels[(U8)b].Priority; });
		std::erase_if(mPriorityPorts, [&](Port& a) { return !mChannels[(U8)a].MasterEnable; });
	}

	U32 DMA::getControlRegister()
	{
		U32 result = 0;

		result |= mChannels[(U8)Port::MDECin].Priority << 0;
		result |= mChannels[(U8)Port::MDECin].MasterEnable << 3;
		result |= mChannels[(U8)Port::MDECout].Priority << 4;
		result |= mChannels[(U8)Port::MDECout].MasterEnable << 7;
		result |= mChannels[(U8)Port::GPU].Priority << 8;
		result |= mChannels[(U8)Port::GPU].MasterEnable << 11;
		result |= mChannels[(U8)Port::CDROM].Priority << 12;
		result |= mChannels[(U8)Port::CDROM].MasterEnable << 15;
		result |= mChannels[(U8)Port::SPU].Priority << 16;
		result |= mChannels[(U8)Port::SPU].MasterEnable << 19;
		result |= mChannels[(U8)Port::PIO].Priority << 20;
		result |= mChannels[(U8)Port::PIO].MasterEnable << 23;
		result |= mChannels[(U8)Port::OTC].Priority << 24;
		result |= mChannels[(U8)Port::OTC].MasterEnable << 27;
		result |= mControlRegister.Dummy1 << 28;
		result |= mControlRegister.Dummy2 << 31;

		return result;
	}

	void DMA::startTransfer(Port port)
	{
		Channel& channel = mChannels[(U8)port];

		channel.Trigger = ESX_FALSE;

		mRunningDMAs++;

		switch (channel.SyncMode) {
			case SyncMode::LinkedList: {
				startLinkedListTransfer(channel);
				break;
			}

			default: {
				startBlockTransfer(channel);
				break;
			}
		}

	}

	void DMA::startBlockTransfer(Channel& channel)
	{
		ESX_CORE_ASSERT(channel.Port != Port::SPU, "DMA Block Transfer Port {} not supported yet", (U8)channel.Port);

		U32 transferSize = 0;
		switch (channel.SyncMode)
		{
			case SyncMode::Manual:
				transferSize = channel.BlockSize;
				break;
			case SyncMode::Blocks:
				transferSize = channel.BlockSize * channel.BlockCount;
				break;
		}

		channel.TransferStatus.BlockCurrentAddress = channel.BaseAddress;
		channel.TransferStatus.BlockRemainingSize = transferSize;
	}

	void DMA::clockBlockTransfer(Channel& channel)
	{
		I32 increment = (channel.Step == Step::Forward) ? 4 : -4;
		SharedPtr<Bus> bus = getBus(ESX_TEXT("Root"));
		SharedPtr<RAM> ram = bus->getDevice<RAM>(ESX_TEXT("RAM"));
		SharedPtr<GPU> gpu = bus->getDevice<GPU>(ESX_TEXT("GPU"));
		SharedPtr<CDROM> cdrom = bus->getDevice<CDROM>(ESX_TEXT("CDROM"));

		U32 currentAddress = channel.TransferStatus.BlockCurrentAddress & 0x1FFFFC;

		switch (channel.Direction) {
			case Direction::ToMainRAM: {
				U32 valueToWrite = 0;

				switch (channel.Port) {
					case Port::OTC: {
						if (channel.TransferStatus.BlockRemainingSize == 1) {
							valueToWrite = 0xFFFFFF;
						}
						else {
							valueToWrite = (channel.TransferStatus.BlockCurrentAddress - 4) & 0x1FFFFF;
						}
						break;
					}

					case Port::GPU: {
						valueToWrite = gpu->gpuRead();
						break;
					}

					case Port::CDROM: {
						U8 b4 = cdrom->popData();
						U8 b3 = cdrom->popData();
						U8 b2 = cdrom->popData();
						U8 b1 = cdrom->popData();

						valueToWrite = (b1 << 24) | (b2 << 16) | (b3 << 8) | (b4 << 0);
						break;
					}

					default: {
						ESX_CORE_LOG_ERROR("Port ToMainRAM {} not supported yet", (U8)channel.Port);
						break;
					}
				}


				ram->store(ESX_TEXT("Root"), currentAddress, valueToWrite);
				break;
			}
			case Direction::FromMainRAM: {
				U32 value = 0;
				ram->load(ESX_TEXT("Root"), currentAddress, value);

				switch (channel.Port) {
					case Port::GPU: {
						gpu->gp0(value);
						break;
					}
					default: {
						ESX_CORE_LOG_ERROR("Port FromMainRAM {} not supported yet", (U8)channel.Port);
						break;
					}
				}

				break;
			}
		}

		channel.TransferStatus.BlockCurrentAddress += increment;
		channel.TransferStatus.BlockRemainingSize--;

		if (channel.TransferStatus.BlockRemainingSize == 0) {
			setChannelDone(channel);
		}
	}


	void DMA::startLinkedListTransfer(Channel& channel)
	{
		ESX_CORE_ASSERT(channel.Port == Port::GPU, "DMA Linked List Port {} not supported yet", (U8)channel.Port);
		ESX_CORE_ASSERT(channel.Direction == Direction::FromMainRAM, "ToMainRAM Direction not supported yet");
		
		channel.TransferStatus.LinkedListCurrentNodeAddress = channel.BaseAddress & 0x1FFFFC;
		channel.TransferStatus.LinkedListCurrentNodeHeader = 0;
		channel.TransferStatus.LinkedListNextNodeAddress = 0;
		channel.TransferStatus.LinkedListRemainingSize = 0;
		channel.TransferStatus.LinkedListPacketAddress = 0;
	}

	void DMA::clockLinkedListTransfer(Channel& channel)
	{
		SharedPtr<Bus> bus = getBus(ESX_TEXT("Root"));
		SharedPtr<RAM> ram = bus->getDevice<RAM>(ESX_TEXT("RAM"));
		SharedPtr<GPU> gpu = bus->getDevice<GPU>(ESX_TEXT("GPU"));
		TransferStatus& transferStatus = channel.TransferStatus;

		if (transferStatus.LinkedListRemainingSize == 0) {
			ram->load(ESX_TEXT("Root"), transferStatus.LinkedListCurrentNodeAddress, transferStatus.LinkedListCurrentNodeHeader);
			transferStatus.LinkedListNextNodeAddress = transferStatus.LinkedListCurrentNodeHeader & 0x1FFFFC;
			transferStatus.LinkedListRemainingSize = transferStatus.LinkedListCurrentNodeHeader >> 24;
			transferStatus.LinkedListPacketAddress = (transferStatus.LinkedListCurrentNodeAddress + 4) & 0x1FFFFC;
		}

		if (transferStatus.LinkedListRemainingSize > 0) {
			U32 packet = 0;
			ram->load(ESX_TEXT("Root"), transferStatus.LinkedListPacketAddress, packet);
			gpu->gp0(packet);

			transferStatus.LinkedListRemainingSize--;
			transferStatus.LinkedListPacketAddress = (transferStatus.LinkedListPacketAddress + 4) & 0x1FFFFC;
		}

		if (transferStatus.LinkedListRemainingSize == 0) {
			if ((transferStatus.LinkedListCurrentNodeHeader & 0x800000) != 0) {
				setChannelDone(channel);
			} else {
				transferStatus.LinkedListCurrentNodeAddress = transferStatus.LinkedListNextNodeAddress;
			}
		}
	}


}