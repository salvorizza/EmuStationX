#include "EXE.h"


namespace esx {

	EXE::EXE(const std::filesystem::path& exePath)
	{
		parse(exePath);
	}

	EXE::~EXE()
	{
		mStream.close();
	}

	void EXE::seek(U64 seekPos)
	{
		mStream.seekg(seekPos, mStream.beg);
	}

	void EXE::readSector(Sector* pOutSector)
	{
		mStream.read(reinterpret_cast<char*>(pOutSector->UserData.data()), pOutSector->UserData.size());
		if (mStream.fail() == ESX_TRUE) {
			ESX_CORE_LOG_TRACE("Strano");
		}
	}

	void EXE::parse(const std::filesystem::path& exePath)
	{
		mStream.open(exePath, std::ios::binary);
	}
}