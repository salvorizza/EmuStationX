#include "MemoryCard.h"

#include "UI/Utils.h"

#include "SIO.h"

namespace esx {



	MemoryCard::MemoryCard()
		: SerialDevice(SerialDeviceType::MemoryCard)
	{
		std::memcpy(mData.Directory.Header.MemoryCardID, "MC", 2);
		mData.Directory.Header.Unused = { 0x00 };
		mData.Directory.Header.Checksum = calculateChecksum((U8*)&mData.Directory.Header, 0x80);

		for (auto& directoryFrame : mData.Directory.DirectoryFrames) {
			directoryFrame.BlockAllocationState = BlockAllocationState::Free;
			directoryFrame.Filesize = 0x00000000;
			directoryFrame.NextBlockNumber = 0x0000;
			std::memset(directoryFrame.Filename, 0x00, 20);
			directoryFrame.Zero = 0x00;
			directoryFrame.Garbage = { 0x00 };
			directoryFrame.Checksum = calculateChecksum((U8*)&directoryFrame, 0x80);
		}

		for (auto& brokenSectorFrame : mData.Directory.BrokenSectorList) {
			brokenSectorFrame.BrokenSectorNumber = 0xFFFFFFFF;
			brokenSectorFrame.Garbage = { 0x00 };
			brokenSectorFrame.Checksum = calculateChecksum((U8*)&brokenSectorFrame, 0x80);
		}

		for (auto& brokenSectorReplacementFrame : mData.Directory.BrokenSectorReplacementData) {
			brokenSectorReplacementFrame.Data = { 0xFF };
		}

		for (auto& unusedFrame : mData.Directory.UnusedFrames) {
			unusedFrame.Unused = { 0xFF };
		}

		std::memcpy(mData.Directory.WriteTestFrame.MemoryCardID, "MC", 2);
		mData.Directory.WriteTestFrame.Unused = { 0x00 };
		mData.Directory.WriteTestFrame.Checksum = calculateChecksum((U8*)&mData.Directory.WriteTestFrame, 0x80);

		std::memcpy(mData.FirstFile.TitleFrame.ID, "SC", 2);
		mData.FirstFile.TitleFrame.IconDisplayFlag = 0x11;
		mData.FirstFile.TitleFrame.BlockNumber = 1;
		std::memset(mData.FirstFile.TitleFrame.TitleShiftJIS, 0x00, 64);
		std::memset(mData.FirstFile.TitleFrame.Reserved0, 0x00, 12);
		std::memset(mData.FirstFile.TitleFrame.Reserved1, 0x00, 16);
		std::memset(mData.FirstFile.TitleFrame.IconPalette, 0x00, 32);

		for (auto& iconFrame : mData.FirstFile.IconFrames) {
			iconFrame.Bitmap = { 0x00 };
		}

		for (auto& file : mData.Files) {
			for (auto& dataFrame : file.DataFrames) {
				dataFrame.Data = { 0x00 };
			}
		}
	}


	MemoryCard::MemoryCard(const std::filesystem::path& path)
		: SerialDevice(SerialDeviceType::MemoryCard)
	{
		DataBuffer file{};
		ReadFile(path.string().c_str(), file);
		std::memcpy(&mData, file.Data, file.Size);
		DeleteBuffer(file);

		mFlag = (1 << 3);
	}

	U8 MemoryCard::receive(U8 value)
	{
		U8 tx = 0xFF;

		switch (mPhase) {
			case MemoryCardCommunicationPhase::Adressing: {
				if (value == 0x81) {
					mPhase = MemoryCardCommunicationPhase::Command;
					mSelected = ESX_TRUE;
					tx = mFlag;
				} else {
					mPhase = MemoryCardCommunicationPhase::NotSelected;
				}
				break;
			}

			case MemoryCardCommunicationPhase::Command: {
				mCurrentCommand = (MemoryCardCommand)value;

				switch (mCurrentCommand) {
					case MemoryCardCommand::Read: {
						mPhases.emplace(MemoryCardCommunicationPhase::ReceiveMemoryCardID1);
						mPhases.emplace(MemoryCardCommunicationPhase::ReceiveMemoryCardID2);
						mPhases.emplace(MemoryCardCommunicationPhase::SendAddressMSB);
						mPhases.emplace(MemoryCardCommunicationPhase::SendAddressLSB);
						mPhases.emplace(MemoryCardCommunicationPhase::ReceiveCommandAcknowledge1);
						mPhases.emplace(MemoryCardCommunicationPhase::ReceiveCommandAcknowledge2);
						mPhases.emplace(MemoryCardCommunicationPhase::ReceiveConfirmedAddressMSB);
						mPhases.emplace(MemoryCardCommunicationPhase::ReceiveConfirmedAddressLSB);
						mPhases.emplace(MemoryCardCommunicationPhase::ReceiveDataSector);
						mPhases.emplace(MemoryCardCommunicationPhase::ReceiveChecksum);
						mPhases.emplace(MemoryCardCommunicationPhase::ReceiveMemoryEndByte);

						break;
					}
					case MemoryCardCommand::Write: {
						mPhases.emplace(MemoryCardCommunicationPhase::ReceiveMemoryCardID1);
						mPhases.emplace(MemoryCardCommunicationPhase::ReceiveMemoryCardID2);
						mPhases.emplace(MemoryCardCommunicationPhase::SendAddressMSB);
						mPhases.emplace(MemoryCardCommunicationPhase::SendAddressLSB);
						mPhases.emplace(MemoryCardCommunicationPhase::SendDataSector);
						mPhases.emplace(MemoryCardCommunicationPhase::SendChecksum);
						mPhases.emplace(MemoryCardCommunicationPhase::ReceiveCommandAcknowledge1);
						mPhases.emplace(MemoryCardCommunicationPhase::ReceiveCommandAcknowledge2);
						mPhases.emplace(MemoryCardCommunicationPhase::ReceiveMemoryEndByte);

						break;
					}
					case MemoryCardCommand::GetID: {
						ESX_CORE_LOG_ERROR("MemoryCardCommand::GetID not implemented yet");
						break;
					}
				}
				tx = 0x5A;
				break;
			}

			case MemoryCardCommunicationPhase::ReceiveMemoryCardID1: {
				tx = 0x5D;
				break;
			}

			case MemoryCardCommunicationPhase::ReceiveMemoryCardID2: {
				tx = 0x00;
				break;
			}

			case MemoryCardCommunicationPhase::SendAddressMSB: {
				mReceivedAddress = value << 8;
				mChecksum ^= value;
				tx = 0x00;
				break;
			}

			case MemoryCardCommunicationPhase::SendAddressLSB: {
				mReceivedAddress |= value;
				mAddressPointer = 0x00;
				mChecksum ^= value;
				if (mCurrentCommand == MemoryCardCommand::Read) {
					tx = 0x5C;
				} else if (mCurrentCommand == MemoryCardCommand::Write) {
					tx = mLastRX;
				}
				break;
			}

			case MemoryCardCommunicationPhase::ReceiveCommandAcknowledge1: {
				tx = 0x5D;
				break;
			}

			case MemoryCardCommunicationPhase::ReceiveCommandAcknowledge2: {
				if (mCurrentCommand == MemoryCardCommand::Read) {
					tx = mReceivedAddress >> 8;
				}
				else if (mCurrentCommand == MemoryCardCommand::Write) {
					mFlag &= ~(1 << 3);
					tx = (mChecksum == mReceivedChecksum) ? 0x47 : 0x4E;
				}
				break;
			}

			case MemoryCardCommunicationPhase::ReceiveConfirmedAddressMSB: {
				tx = mReceivedAddress & 0xFF;
				break;
			}

			case MemoryCardCommunicationPhase::ReceiveConfirmedAddressLSB: {
				tx = ((U8*)&mData)[(mReceivedAddress * 128) + mAddressPointer];
				mChecksum ^= tx;
				mAddressPointer++;
				break;
			}

			case MemoryCardCommunicationPhase::ReceiveDataSector: {
				if (mAddressPointer < 128) {
					tx = ((U8*)&mData)[(mReceivedAddress * 128) + mAddressPointer];
					mChecksum ^= tx;
				} else {
					tx = mChecksum;
				}
				mAddressPointer++;
				break;
			}

			case MemoryCardCommunicationPhase::SendDataSector: {
				if (mAddressPointer < 128) {
					((U8*)&mData)[(mReceivedAddress * 128) + mAddressPointer] = value;
					mChecksum ^= value;
				}
				mAddressPointer++;
				tx = mLastRX;
				break;
			}

			case MemoryCardCommunicationPhase::ReceiveChecksum: {
				tx = 0x47;
				break;
			}

			case MemoryCardCommunicationPhase::SendChecksum: {
				mReceivedChecksum = value;
				tx = 0x5C;
				break;
			}

		}

		if (!mPhases.empty()) {
			if (((mPhase == MemoryCardCommunicationPhase::ReceiveDataSector && mAddressPointer > 128) || (mPhase == MemoryCardCommunicationPhase::SendDataSector && mAddressPointer >= 128))
				|| (mPhase != MemoryCardCommunicationPhase::ReceiveDataSector && mPhase != MemoryCardCommunicationPhase::SendDataSector)) {
				mPhase = mPhases.front();
				mPhases.pop();
			}
		}

		if (mSelected) {
			mMaster->dsr();
		}

		mLastRX = value;

		return tx;
	}

	void MemoryCard::cs()
	{
		mPhase = MemoryCardCommunicationPhase::Adressing;
		mTX.Set(0xFF);
		mRX = {};
		mSelected = ESX_FALSE;
		mPhases = {};
		mChecksum = 0;
	}

	void MemoryCard::Save()
	{
		DataBuffer buffer((U8*)&mData, sizeof(MemoryCardData));
		WriteFile("commons/memory_cards/test.mcr", buffer);
	}

	U8 MemoryCard::calculateChecksum(U8* frame, U64 size)
	{
		U8 checksum = 0;
		for (U8 i = 0; i < size - 1; i++) {
			checksum ^= frame[i];
		}
		return checksum;
	}

}