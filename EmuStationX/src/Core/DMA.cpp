#include "DMA.h"

namespace esx {

	DMA::DMA()
		: BusDevice(ESX_TEXT("DMA"))
	{
		addRange(ESX_TEXT("Root"), 0x1F801080, BYTE(0x75), 0xFFFFFFFF);

		setControlRegister(0x07654321);
		setInterruptRegister(0);
	}

	DMA::~DMA()
	{
	}

	void DMA::store(const String& busName, U32 address, U32 value)
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
				Port channel = (Port)(((address >> 4) & 0xF) - 0x8);

				switch (address & 0xF) {
					case 0x0: {
						setChannelBaseAddress(channel, value);
						break;
					}
					case 0x4: {
						setChannelBlockControl(channel, value);
						break;
					}
					case 0x8: {
						setChannelControl(channel, value);
						break;
					}
				}

				if (isChannelActive(channel)) {
					startTransfer(channel);
				}
			}
		}
	}

	void DMA::load(const String& busName, U32 address, U32& output)
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

	bool DMA::isChannelActive(Port port)
	{
		bool trigger = true;
		if (mChannels[(U8)port].SyncMode == SyncMode::Manual) {
			trigger = mChannels[(U8)port].Trigger;
		}

		return mChannels[(U8)port].Enable && trigger;
	}

	void DMA::setChannelDone(Port port)
	{
		mChannels[(U8)port].Enable = false;
		mChannels[(U8)port].Trigger = false;
	}

	void DMA::setInterruptRegister(U32 value)
	{
		mInterruptRegister.Dummy = (value >> 0) & 0x1F;
		mInterruptRegister.ForceIRQ = (value >> 15) & 0x1;
		mInterruptRegister.IRQEnable = (value >> 16) & 0x7F;
		mInterruptRegister.IRQMasterEnable = (value >> 23) & 0x1;
		mInterruptRegister.IRQFlags = (value >> 24) & 0x7F;
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
		mControlRegister.MDECinPriority = (value >> 0) & 0x3;
		mControlRegister.MDECinMasterEnable = (value >> 3) & 0x1;
		mControlRegister.MDECoutPriority = (value >> 4) & 0x3;
		mControlRegister.MDECoutMasterEnable = (value >> 7) & 0x1;
		mControlRegister.GPUPriority = (value >> 8) & 0x3;
		mControlRegister.GPUMasterEnable = (value >> 11) & 0x1;
		mControlRegister.CDROMPriority = (value >> 12) & 0x3;
		mControlRegister.CDROMMasterEnable = (value >> 15) & 0x1;
		mControlRegister.SPUPriority = (value >> 16) & 0x3;
		mControlRegister.SPUMasterEnable = (value >> 19) & 0x1;
		mControlRegister.PIOPriority = (value >> 20) & 0x3;
		mControlRegister.PIOMasterEnable = (value >> 23) & 0x1;
		mControlRegister.OTCPriority = (value >> 24) & 0x3;
		mControlRegister.OTCMasterEnable = (value >> 27) & 0x1;
		mControlRegister.Dummy1 = (value >> 28) & 0x3;
		mControlRegister.Dummy2 = (value >> 31) & 0x1;
	}

	U32 DMA::getControlRegister()
	{
		U32 result = 0;

		result |= mControlRegister.MDECinPriority << 0;
		result |= mControlRegister.MDECinMasterEnable << 3;
		result |= mControlRegister.MDECoutPriority << 4;
		result |= mControlRegister.MDECoutMasterEnable << 7;
		result |= mControlRegister.GPUPriority << 8;
		result |= mControlRegister.GPUMasterEnable << 11;
		result |= mControlRegister.CDROMPriority << 12;
		result |= mControlRegister.CDROMMasterEnable << 15;
		result |= mControlRegister.SPUPriority << 16;
		result |= mControlRegister.SPUMasterEnable << 19;
		result |= mControlRegister.PIOPriority << 20;
		result |= mControlRegister.PIOMasterEnable << 23;
		result |= mControlRegister.OTCPriority << 24;
		result |= mControlRegister.OTCMasterEnable << 27;
		result |= mControlRegister.Dummy1 << 28;
		result |= mControlRegister.Dummy2 << 31;

		return result;
	}

	void DMA::startTransfer(Port port)
	{
		const Channel& channel = mChannels[(U8)port];

		switch (channel.SyncMode) {
			case SyncMode::LinkedList: {
				startLinkedListTransfer(port, channel);
				break;
			}

			default: {
				startBlockTransfer(port, channel);
				break;
			}
		}

	}

	void DMA::startBlockTransfer(Port port, const Channel& channel)
	{
		U32 address = channel.BaseAddress;

		I32 increment = (channel.Step == Step::Forward) ? 4 : -4;

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

		Bus* bus = getBus(ESX_TEXT("Root"));
		RAM* ram = bus->getDevice<RAM>(ESX_TEXT("RAM"));

		ESX_CORE_LOG_TRACE("\nStarting Block DMA:\n\tPort: {}\n\tBase Address: {:08x}\n\tSize: {}\n\tIncrement: {}\n\tDirection: {}", (U8)port, address, transferSize, increment, (U8)channel.Direction);
		for (I32 remainingSize = transferSize; remainingSize > 0; remainingSize--) {
			U32 currentAddress = address & 0x1FFFFC;

			switch (channel.Direction) {
				case Direction::ToMainRAM: {
					U32 valueToWrite = 0;

					switch (port) {
						case Port::OTC: {
							if (remainingSize == 1) {
								valueToWrite = 0xFFFFFF;
							}
							else {
								valueToWrite = (address - 4) & 0x1FFFFF;
							}
							break;
						}
						default: {
							ESX_CORE_ASSERT(false, "Port {} not supported yet", (U8)port);
							break;
						}
					}


					ram->store(ESX_TEXT("Root"), currentAddress, valueToWrite);
					break;
				}
				case Direction::FromMainRAM: {
					U32 value = 0;
					ram->load(ESX_TEXT("Root"), currentAddress, value);

					switch (port) {
						case Port::GPU: {
							ESX_CORE_LOG_TRACE("GPU Data: {:08x}", value);
							break;
						}
						default: {
							ESX_CORE_ASSERT(false, "Port {} not supported yet", (U8)port);
							break;
						}
					}

					break;
				}
			}

			address += increment;
		}

		setChannelDone(port);
		ESX_CORE_LOG_TRACE("DMA Done");
	}

	void DMA::startLinkedListTransfer(Port port, const Channel& channel)
	{
		ESX_CORE_ASSERT(port == Port::GPU, "DMA Port {} not supported yet", (U8)port);
		ESX_CORE_ASSERT(channel.Direction == Direction::FromMainRAM, "ToMainRAM Direction not supported yet");

		Bus* bus = getBus(ESX_TEXT("Root"));
		RAM* ram = bus->getDevice<RAM>(ESX_TEXT("RAM"));

		U32 nodeAddress = channel.BaseAddress & 0x1FFFFC;
		U32 header = 0, packet = 0;

		ESX_CORE_LOG_TRACE("\nStarting Linked List DMA:\n\tPort: {}\n\tStart Address: {:08x}", (U8)port, nodeAddress);

		do
		{
			ram->load(ESX_TEXT("Root"), nodeAddress, header);

			U32 nextNodeAddress = header & 0x1FFFFC;
			I32 extraWords = header >> 24;

			U32 packetAddress = (nodeAddress + 4) & 0x1FFFFC;
			for (I32 remainingSize = extraWords; remainingSize > 0; remainingSize--) {
				ram->load(ESX_TEXT("Root"), packetAddress, packet);

				ESX_CORE_LOG_TRACE("GPU Command: {:8x}", packet);

				packetAddress = (packetAddress + 4) & 0x1FFFFC;
			}

			if ((header & 0x800000) != 0) {
				break;
			}

			nodeAddress = nextNodeAddress;
		} while (true);

		setChannelDone(port);
		ESX_CORE_LOG_TRACE("DMA Done");
	}


}