#include "MemoryCard.h"

#include "UI/Utils.h"

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
	}

	U8 MemoryCard::receive(U8 value)
	{
		return 0;
	}

	void MemoryCard::cs()
	{}

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