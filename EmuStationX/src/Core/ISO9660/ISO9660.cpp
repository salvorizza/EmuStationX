#include "ISO9660.h"

namespace esx {



	ISO9660::ISO9660(const SharedPtr<CompactDisk>& cd)
		: mCD(cd)
	{
		ReadPrimaryVolumeDecriptor();
		ReadPathTable();
	}

	Vector<String> ISO9660::ListDirectories(const StringView& path)
	{
		Vector<String> result = {};

		ReadPathTable();
		FileSystemDirectory* currentNode = SearchDirectory(path);
		if (currentNode) {
			for (auto& child : currentNode->ChildDirectories) {
				result.push_back(String(path.data()) + (path.ends_with('\\') ? "" : "\\") + child.second.PathTableRecord.DirectoryName);
			}
		}

		return result;
	}

	const DirectoryRecord& ISO9660::GetDirectoryInfo(const StringView& path)
	{
		ReadPathTable();
		FileSystemDirectory* currentNode = SearchDirectory(path);
		ReadDirectoryEntries(currentNode);
		return currentNode ? currentNode->DirectoryInfo : DirectoryRecord();
	}

	Vector<String> ISO9660::ListFiles(const StringView& directoryPath)
	{
		Vector<String> result = {};

		ReadPathTable();
		FileSystemDirectory* currentNode = SearchDirectory(directoryPath);
		if (currentNode) {
			ReadDirectoryEntries(currentNode);
			for (auto& child : currentNode->Files) {
				result.push_back(String(directoryPath.data()) + (directoryPath.ends_with('\\') ? "" : "\\") + child.second.Name);
			}
		}

		return result;
	}

	const DirectoryRecord& ISO9660::GetFileInfo(const StringView& filePath)
	{
		ReadPathTable();
		return SearchFile(filePath);
	}

	void ISO9660::GetFileData(const StringView& filePath, Vector<U8>& data)
	{
		ReadPathTable();
		const DirectoryRecord& file = SearchFile(filePath);
		ReadFileData(file, data);
	}

	void ISO9660::ReadPrimaryVolumeDecriptor()
	{
		Sector pvd = {};
		mCD->seek(CompactDisk::calculateBinaryPosition(0, 2, 16));
		mCD->readSector(&pvd);
		std::memcpy(&mPVD, pvd.UserData.data(), sizeof(PrimaryVolumeDescriptor));
	}

	void ISO9660::ReadPathTable()
	{
		if (mFileSystem.Number != 0) return;

		Sector pathTable = {};
		mCD->seek(CompactDisk::calculateBinaryPosition(0, 2, mPVD.PathTable1BlockNumberLE));
		mCD->readSector(&pathTable);

		FileSystemDirectory* currentNode = &mFileSystem;
		BIT firstIteration = ESX_TRUE;
		I32 directoryNumber = 1;
		String lastChildName = "";
		char directoryNameBuffer[9] = "\0";
		UnorderedMap<U32, FileSystemDirectory*> dirMap = {};
		for (I32 bufferPointer = 0; bufferPointer < mPVD.PathTableSizeLE;) {
			FileSystemDirectory node;

			node.Number = directoryNumber;
			node.ChildDirectories = {};

			U8* buffer = &pathTable.UserData[bufferPointer];
			U64 copySize = offsetof(PathTableRecord, DirectoryName);
			std::memcpy(&node.PathTableRecord, buffer, copySize);
			bufferPointer += copySize;
			buffer += copySize;

			buffer = &pathTable.UserData[bufferPointer];
			copySize = node.PathTableRecord.LengthOfDirectoryName;
			memset(directoryNameBuffer, 0, 8);
			std::memcpy(directoryNameBuffer, buffer, copySize);
			bufferPointer += copySize;
			buffer += copySize;

			node.PathTableRecord.DirectoryName = directoryNameBuffer;

			if ((node.PathTableRecord.LengthOfDirectoryName % 2) != 0) {
				bufferPointer++;
			}


			FileSystemDirectory* refNode;
			if (firstIteration) {
				*currentNode = node;
				dirMap[node.Number] = currentNode;
			} else {
				if (dirMap.contains(node.PathTableRecord.ParentDirectoryNumber)) {
					dirMap[node.PathTableRecord.ParentDirectoryNumber]->ChildDirectories[node.PathTableRecord.DirectoryName] = node;
					dirMap[node.Number] = &(dirMap[node.PathTableRecord.ParentDirectoryNumber]->ChildDirectories[node.PathTableRecord.DirectoryName]);
				}
				else {
					currentNode = &currentNode->ChildDirectories[lastChildName];
					dirMap[node.Number] = &(currentNode->ChildDirectories[lastChildName]);
				}
			}


			lastChildName = node.PathTableRecord.DirectoryName;
			firstIteration = ESX_FALSE;
			directoryNumber++;
		}
	}

	void ISO9660::ReadDirectoryEntries(FileSystemDirectory* directory)
	{
		if (directory->DirectoryInfo.LengthOfDirectoryRecord != 0) return;

		Sector directoryRecords = {};
		U64 binPos = CompactDisk::calculateBinaryPosition(0, 2, directory->PathTableRecord.DirectoryLogicalBlockNumber);
		mCD->seek(binPos);
		mCD->readSector(&directoryRecords);

		I32 bufferPointer = 0;
		char name[64] = "\0";
		I32 directoryIndex = 0;
		while (true) {
			DirectoryRecord record;
			U8 padding = 0;

			U8* buffer = &directoryRecords.UserData[bufferPointer];
			U64 copySize = offsetof(DirectoryRecord, Name);
			std::memcpy(&record, buffer, copySize);
			if (record.LengthOfDirectoryRecord != 0) {
				bufferPointer += copySize;
				buffer += copySize;

				buffer = &directoryRecords.UserData[bufferPointer];
				copySize = record.LengthOfName;
				memset(name, 0, 14);
				std::memcpy(name, buffer, copySize);
				bufferPointer += copySize;
				buffer += copySize;

				record.Name = name;

				if ((record.LengthOfName % 2) == 0) {
					bufferPointer++;
					padding = 1;
				}

				buffer = &directoryRecords.UserData[bufferPointer];
				copySize = record.LengthOfDirectoryRecord - (33 + record.LengthOfName + padding);
				record.SystemUse.resize(copySize);
				std::memcpy(record.SystemUse.data(), buffer, copySize);
				bufferPointer += copySize;
				buffer += copySize;

				if (directoryIndex == 0) {
					directory->DirectoryInfo = record;
				}

				if (record.FileFlags == 0) {
					directory->Files[record.Name] = record;
				}

				directoryIndex++;
			} else {
				bufferPointer++;
			}

			if (bufferPointer >= directory->DirectoryInfo.DataSizeLE) {
				break;
			}
		}
	}

	void ISO9660::ReadFileData(const DirectoryRecord& file, Vector<U8>& data)
	{
		Sector fileData = {};
		
		U32 sector = file.DataLogicalBlockNumberLE;
		U32 numSectors = data.size() / 0x800;
		if ((data.size() % 0x800) != 0) {
			numSectors++;
		}

		U32 bufferPointer = 0;
		U32 remainingSize = data.size();
		for (I32 i = 0; i < numSectors; i++) {
			Sector fileData = {};
			U64 binPos = CompactDisk::calculateBinaryPosition(0, 2, file.DataLogicalBlockNumberLE + i);
			mCD->seek(binPos);
			mCD->readSector(&fileData);

			U32 sizeToCopy = std::min(remainingSize, 0x800u);
			std::memcpy(&data[bufferPointer], fileData.UserData.data(), sizeToCopy);
			remainingSize -= sizeToCopy;
			bufferPointer += sizeToCopy;
		}
	}

	FileSystemDirectory* ISO9660::SearchDirectory(const StringView& path)
	{
		StringStream tokenizer = StringStream(String(path));
		String token = "";
		Vector<String> tokens;
		while (std::getline(tokenizer, token, '\\')) {
			tokens.push_back(token + "\0");
		}

		FileSystemDirectory* currentNode = &mFileSystem;
		BIT found = ESX_TRUE;
		for (I32 i = 0; i < tokens.size(); i++) {
			auto& token = tokens[i];

			if (i == 0) {
				if (token != "") {
					found = ESX_FALSE;
					break;
				}
				else {
					continue;
				}
			}

			if (!currentNode->ChildDirectories.contains(token)) {
				found = ESX_FALSE;
				break;
			}

			currentNode = &currentNode->ChildDirectories[token];
		}

		return found ? currentNode : nullptr;
	}

	const DirectoryRecord& ISO9660::SearchFile(const StringView& filePath)
	{
		size_t last = filePath.find_last_of("\\");
		auto directoryPath = filePath.substr(0, last);
		auto fileName = String(filePath.substr(last + 1, filePath.size()).data());

		FileSystemDirectory* directory = SearchDirectory(directoryPath);
		ReadDirectoryEntries(directory);
		if (directory) {
			if (directory->Files.contains(fileName)) {
				return directory->Files[fileName];
			}
		}

		return {};
	}

	String ISO9660::NormalizePath(const StringView& path)
	{
		String copy = path.data();

		if (!copy.starts_with("\\")) {
			copy = "\\" + copy;
		}

		return copy;
	}

}