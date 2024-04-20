#pragma once

#include "Base/Base.h"
#include "Utils/LoggingSystem.h"
#include "SerialDevice.h"

namespace esx {

	struct HeaderFrame {
		Char MemoryCardID[2];
		Array<U8, 0x7D> Unused;
		U8 Checksum;
	};

	enum class BlockAllocationState : U32 {
		FirstBlockInUse = 0x51,
		MiddleBlockInUse = 0x52,
		LastBlockInUse = 0x53,
		Free = 0xA0,
		FirstBlockFree = 0xA1,
		MiddleBlockFree = 0xA2,
		LastBlockFree = 0xA3
	};

	struct DirectoryFrame {
		BlockAllocationState BlockAllocationState;
		U32 Filesize;
		U16 NextBlockNumber;
		Char Filename[20];
		U8 Zero;
		Array<U8, 0x5F> Garbage;
		U8 Checksum;
	};

	struct BrokenSectorFrame {
		U32 BrokenSectorNumber;
		Array<U8, 0x7B> Garbage;
		U8 Checksum;
	};

	struct BrokenSectorReplacementFrame {
		Array<U8, 0x80> Data;
	};

	struct UnusedFrame {
		Array<U8, 0x80> Unused;
	};

	struct TitleFrame {
		Char ID[2];
		U8 IconDisplayFlag;
		U8 BlockNumber;
		Char TitleShiftJIS[64];
		U8 Reserved0[12];
		U8 Reserved1[16];
		U8 IconPalette[32];
	};

	struct IconFrame {
		Array<U8, 0x80> Bitmap;
	};

	struct DataFrame {
		Array<U8, 0x80> Data;
	};

	struct FirstFile {
		TitleFrame TitleFrame;
		Array<IconFrame, 3> IconFrames;
		Array<DataFrame, 60> DataFrames;
	};

	struct File {
		Array<DataFrame, 64> DataFrames;
	};

	struct Directory {
		HeaderFrame Header;
		Array<DirectoryFrame, 15> DirectoryFrames;
		Array<BrokenSectorFrame, 20> BrokenSectorList;
		Array<BrokenSectorReplacementFrame, 20> BrokenSectorReplacementData;
		Array<UnusedFrame, 7> UnusedFrames;
		HeaderFrame WriteTestFrame;
	};

	struct MemoryCardData {
		Directory Directory;
		FirstFile FirstFile;
		Array<File, 14> Files;
	};

	enum class MemoryCardCommunicationPhase {
		Adressing,
		Command,
		ReceiveMemoryCardID1,
		ReceiveMemoryCardID2,
		SendAddressMSB,
		SendAddressLSB,
		ReceiveCommandAcknowledge1,
		ReceiveCommandAcknowledge2,
		ReceiveConfirmedAddressMSB,
		ReceiveConfirmedAddressLSB,
		ReceiveDataSector,
		SendDataSector,
		ReceiveChecksum,
		SendChecksum,
		ReceiveMemoryEndByte
	};

	enum class MemoryCardCommand : U8 {
		None,
		Read = 'R',
		Write = 'W',
		GetID = 'S'
	};

	class MemoryCard : public SerialDevice {
	public:
		MemoryCard();
		MemoryCard(const std::filesystem::path& path);
		virtual ~MemoryCard() = default;

		U8 receive(U8 value) override;
		void cs() override;

		void Save();

	private:
		U8 calculateChecksum(U8* frame, U64 frameSize);
	private:
		MemoryCardData mData = {};
		U8 mFlag = 0x00;
		U16 mReceivedAddress = 0x0000;
		U8 mAddressPointer = 0x00;
		U8 mChecksum = 0x00;
		U8 mReceivedChecksum = 0x00;
		U8 mLastRX = 0x00;

		MemoryCardCommand mCurrentCommand = MemoryCardCommand::None;
		MemoryCardCommunicationPhase mPhase = MemoryCardCommunicationPhase::Adressing;
		Queue<MemoryCardCommunicationPhase> mPhases;
	};

}