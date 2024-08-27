#include "DMA.h"

#include "Core/InterruptControl.h"
#include "Core/RAM.h"
#include "Core/GPU.h"
#include "Core/CDROM.h"
#include "Core/MDEC.h"
#include "Core/SPU.h"

namespace esx {

	DMA::DMA()
		: BusDevice(ESX_TEXT("DMA"))
	{
		addRange(ESX_TEXT("Root"), 0x1F801080, BYTE(0x7F), 0xFFFFFFFF);
	}

	DMA::~DMA()
	{
	}

	void DMA::clock(U64 clocks)
	{
		constexpr Array<U64, 7> clocks_per_word = {
			1,
			1,
			1,
			40,
			4,
			20,
			1
		};

		if (mRunningDMAs == 0) return;
		if (mPriorityPorts.size() == 0) return;

		for (Port& port : mPriorityPorts) {
			Channel& channel = mChannels[(U8)port];

			if (channel.MasterEnable && channel.TransferStartOrBusy && (clocks % clocks_per_word[(U8)port]) == 0) {
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
				switch (address) {
					case 0x1F8010F0: {
						setControlRegister(value);
						break;
					}
					case 0x1F8010F4: {
						setInterruptRegister(value);
						break;
					}
					default: {
						ESX_CORE_LOG_WARNING("DMA - Reading from address {:08x} not implemented yet", address);
						break;
					}
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
			}
		}
	}

	void DMA::load(const StringView& busName, U32 address, U32& output)
	{

		switch (address & (~0xF)) {
			case 0x1F8010F0: {
				switch (address) {
					case 0x1F8010F0: {
						output = getControlRegister();
						break;
					}
					case 0x1F8010F4: {
						output = getInterruptRegister();
						break;
					}
					default: {
						ESX_CORE_LOG_WARNING("DMA - Reading from address {:08x} not implemented yet", address);
						break;
					}
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

	void DMA::store(const StringView& busName, U32 address, U8 value)
	{
		U32 alignedAddress = address & ~0x3;
		U8 align = address & 0x3;

		U32 word = static_cast<U32>(value << (align * 8));

		store(busName, alignedAddress, word);
	}

	void DMA::load(const StringView& busName, U32 address, U8& output)
	{
		U32 wordOutput = 0;

		U32 alignedAddress = address & ~0x3;
		U8 align = address & 0x3;

		load(busName, alignedAddress, wordOutput);

		output = static_cast<U8>((wordOutput >> (align * 8)) & 0xFF);
	}

	void DMA::reset()
	{
		mControlRegister = {};
		mInterruptRegister = {};
		mPriorityPorts = {
			Port::MDECin,
			Port::MDECout,
			Port::GPU,
			Port::CDROM,
			Port::SPU,
			Port::PIO,
			Port::OTC
		};
		mRunningDMAs = 0;


		setControlRegister(0x07654321);
		setInterruptRegister(0);

		mChannels = {};
		for (U8 port = 0; port < (U8)Port::Max; port++) {
			mChannels[port].Port = (Port)port;
		}

		mBus = getBus(ESX_TEXT("Root"));
		mRAM = mBus->getDevice<RAM>(ESX_TEXT("RAM"));
		mGPU = mBus->getDevice<GPU>(ESX_TEXT("GPU"));
		mCDROM = mBus->getDevice<CDROM>(ESX_TEXT("CDROM"));
		mMDEC = mBus->getDevice<MDEC>(ESX_TEXT("MDEC"));
		mSPU = mBus->getDevice<SPU>(ESX_TEXT("SPU"));
		mInterruptControl = mBus->getDevice<InterruptControl>("InterruptControl");
	}

	void DMA::setChannelControl(Port port, U32 channelControl)
	{
		mChannels[(U8)port].Direction = (Direction)((channelControl >> 0) & 0x1);
		mChannels[(U8)port].Step = (Step)((channelControl >> 1) & 0x1);
		mChannels[(U8)port].ChoppingEnable = ((channelControl >> 8) & 0x1);
		mChannels[(U8)port].SyncMode = (SyncMode)((channelControl >> 9) & 0x3);
		mChannels[(U8)port].ChoppingDMAWindowSize = ((channelControl >> 16) & 0x7);
		mChannels[(U8)port].ChoppingCPUWindowSize = ((channelControl >> 20) & 0x7);
		mChannels[(U8)port].TransferStartOrBusy = ((channelControl >> 24) & 0x1);
		mChannels[(U8)port].ForceTransferStart = ((channelControl >> 28) & 0x1);
		mChannels[(U8)port].Dummy = ((channelControl >> 29) & 0x3);

		if (mChannels[(U8)port].ChoppingEnable) {
			ESX_CORE_LOG_ERROR("DMA - Chopping not implemented yet");
		}

		if (isChannelActive(port)) {
			startTransfer(port);
		}
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
		result |= ((U8)mChannels[(U8)port].TransferStartOrBusy) << 24;
		result |= ((U8)mChannels[(U8)port].ForceTransferStart) << 28;
		result |= ((U8)mChannels[(U8)port].Dummy) << 29;

		return result;
	}

	void DMA::setChannelBaseAddress(Port port, U32 value)
	{
		mChannels[(U8)port].BaseAddress = value & 0xFFFFFF;
		mChannels[(U8)port].BaseAddress &= ~0x3;
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
		if (channel.SyncMode == SyncMode::Burst) {
			trigger = channel.ForceTransferStart;
		}

		return (channel.MasterEnable && channel.TransferStartOrBusy && trigger) ? ESX_TRUE : ESX_FALSE;
	}

	void DMA::setChannelDone(Channel& channel)
	{
		channel.TransferStartOrBusy = ESX_FALSE;

		U8 flag = 1 << (U8)channel.Port;

		
		if (mInterruptRegister.IRQEnable & flag) {
			BIT oldIRQ = mInterruptRegister.IRQMasterFlag();
			mInterruptRegister.IRQFlags |= flag;
			BIT newIRQ = mInterruptRegister.IRQMasterFlag();

			mInterruptControl->requestInterrupt(InterruptType::DMA, oldIRQ, newIRQ);
		}

		mRunningDMAs--;
	}

	void DMA::setInterruptRegister(U32 value)
	{
		BIT oldIRQ = mInterruptRegister.IRQMasterFlag();

		U8 IRQResetMask = ~((value >> 24) & 0x7F);

		mInterruptRegister.IRQCompletionInterrupt = (value >> 0) & 0x7F;
		mInterruptRegister.Unused = (value >> 7) & 0xFF;
		mInterruptRegister.BusErrorFlag = (value >> 15) & 0x1;
		mInterruptRegister.IRQEnable = (value >> 16) & 0x7F;
		mInterruptRegister.IRQMasterEnable = (value >> 23) & 0x1;
		mInterruptRegister.IRQFlags &= IRQResetMask;

		mInterruptControl->requestInterrupt(InterruptType::DMA, oldIRQ, mInterruptRegister.IRQMasterFlag());

		ESX_CORE_LOG_TRACE("IRQEnable => {:06b}b, IRQFlags => {:06b}b", mInterruptRegister.IRQEnable, mInterruptRegister.IRQFlags);
	}

	U32 DMA::getInterruptRegister()
	{
		U32 result = 0;

		result |= mInterruptRegister.IRQCompletionInterrupt << 0;
		result |= mInterruptRegister.Unused << 7;
		result |= mInterruptRegister.BusErrorFlag << 15;
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

		channel.ForceTransferStart = ESX_FALSE;

		mRunningDMAs++;

		if ((mInterruptRegister.IRQCompletionInterrupt >> (U8)port) & 0x1) {
			ESX_CORE_LOG_ERROR("Every slice interrupt not implemented yet");
		}

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
		U32 transferSize = 0;
		switch (channel.SyncMode)
		{
			case SyncMode::Burst:
				transferSize = channel.BlockSize;
				break;
			case SyncMode::Slice:
				transferSize = channel.BlockSize * channel.BlockCount;
				break;
		}

		channel.TransferStatus.BlockCurrentAddress = channel.BaseAddress;
		channel.TransferStatus.BlockRemainingSize = transferSize;

		ESX_CORE_LOG_TRACE("DMA - Starting Block Transfer of {:08x}h size with starting address {:08x}h on port {} direction {}", channel.TransferStatus.BlockRemainingSize, channel.TransferStatus.BlockCurrentAddress, (U8)channel.Port, (channel.Direction == Direction::ToMainRAM) ? "ToMainRAM" : "FromMainRAM");
	}

	void DMA::clockBlockTransfer(Channel& channel)
	{
		I32 increment = (channel.Step == Step::Forward) ? 4 : -4;


		U32 currentAddress = channel.TransferStatus.BlockCurrentAddress & 0x1FFFFC;

		switch (channel.Direction) {
			case Direction::ToMainRAM: {
				U32 valueToWrite = 0;

				switch (channel.Port) {
					case Port::MDECout: {
						valueToWrite = mMDEC->channelOut();
						break;
					}

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
						valueToWrite = mGPU->gpuRead();
						break;
					}

					case Port::CDROM: {
						U8 b4 = mCDROM->popData();
						U8 b3 = mCDROM->popData();
						U8 b2 = mCDROM->popData();
						U8 b1 = mCDROM->popData();

						valueToWrite = (b1 << 24) | (b2 << 16) | (b3 << 8) | (b4 << 0);
						break;
					}

					case Port::SPU: {
						ESX_CORE_LOG_ERROR("SPU Read DMA {} not supported yet", (U8)channel.Port);
						break;
					}

					default: {
						ESX_CORE_LOG_ERROR("Port ToMainRAM {} not supported yet", (U8)channel.Port);
						break;
					}
				}

				mRAM->store(ESX_TEXT("Root"), currentAddress, valueToWrite);
				break;
			}
			case Direction::FromMainRAM: {
				U32 value = 0;
				mRAM->load(ESX_TEXT("Root"), currentAddress, value);

				switch (channel.Port) {
					case Port::MDECin: {
						mMDEC->channelIn(value);
						break;
					}
					case Port::SPU: {
						mSPU->writeToRAM(value);
						break;
					}
					case Port::GPU: {
						mGPU->gp0(value);
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
		if(channel.TransferStatus.BlockRemainingSize > 0) channel.TransferStatus.BlockRemainingSize--;

		if (channel.SyncMode == SyncMode::Slice) {
			channel.BaseAddress = channel.TransferStatus.BlockCurrentAddress;
		}

		if (channel.TransferStatus.BlockRemainingSize == 0) {
			//ESX_CORE_LOG_TRACE("DMA - Block Transfer Done");
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

		//ESX_CORE_LOG_TRACE("DMA - Starting Linked List Transfer starting node {:08x}h on port {}", channel.TransferStatus.LinkedListCurrentNodeAddress, (U8)channel.Port);
	}

	void DMA::clockLinkedListTransfer(Channel& channel)
	{
		TransferStatus& transferStatus = channel.TransferStatus;


		if (transferStatus.LinkedListRemainingSize == 0) {
			mRAM->load(ESX_TEXT("Root"), transferStatus.LinkedListCurrentNodeAddress, transferStatus.LinkedListCurrentNodeHeader);
			transferStatus.LinkedListNextNodeAddress = transferStatus.LinkedListCurrentNodeHeader & 0x1FFFFC;
			transferStatus.LinkedListRemainingSize = transferStatus.LinkedListCurrentNodeHeader >> 24;
			transferStatus.LinkedListPacketAddress = (transferStatus.LinkedListCurrentNodeAddress + 4) & 0x1FFFFC;
		}

		if (transferStatus.LinkedListRemainingSize > 0) {
			U32 packet = 0;
			mRAM->load(ESX_TEXT("Root"), transferStatus.LinkedListPacketAddress, packet);
			mGPU->gp0(packet);

			transferStatus.LinkedListRemainingSize--;
			transferStatus.LinkedListPacketAddress = (transferStatus.LinkedListPacketAddress + 4) & 0x1FFFFC;
		}

		if (transferStatus.LinkedListRemainingSize == 0) {
			if ((transferStatus.LinkedListCurrentNodeHeader & 0x800000) != 0) {
				setChannelDone(channel);
			} else {
				transferStatus.LinkedListCurrentNodeAddress = transferStatus.LinkedListNextNodeAddress;
				channel.BaseAddress = transferStatus.LinkedListCurrentNodeAddress;
			}
		}
	}


}