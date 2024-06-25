#pragma once

#include <filesystem>

#include "Base/Base.h"
#include "Utils/LoggingSystem.h"

#include "CompactDisk.h"

namespace esx {

	struct EXEHeader {
		Char ID[8] = {};
		Array<U8, 8> Zerofilled0 = {};
		U32 InitialPC = 0;
		U32 InitialGP = 0;
		U32 DestinationAddressInRAM = 0;
		U32 FileSize = 0;
		U32 DataSectionStartAddress = 0;
		U32 DataSectionSizeInBytes = 0;
		U32 BSSSectionStartAddress = 0;
		U32 BSSSectionSizeInBytes = 0;
		U32 InitialSP_FP_Base = 0;
		U32 InitialSP_FP_Offset = 0;
		Array<U8, 0x14> Reserved = {};
		Array<U8, 0x7B4> ASCIIMarker_ZeroFilled = {};
	};

	constexpr size_t p = sizeof(EXEHeader);

	class EXE : public CompactDisk {
	public:
		EXE(const std::filesystem::path& exePath);
		~EXE();

		virtual void seek(U8 minute, U8 second, U8 sector) override;
		virtual void readSector(Sector* pOutSector) override;

	private:
		void parse(const std::filesystem::path& exePath);

	private:
		StringView mEXEPath;
		FileInputStream mStream = {};
		U64 mSize = 0;
	};

}