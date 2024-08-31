#pragma once

#include "Base/Base.h"

#include "Core/CD/CompactDisk.h"

namespace esx {

#pragma pack(push, 1) // Assicura che il compilatore non aggiunga padding tra i membri della struttura

    struct DirectoryRecord {
        U8  LengthOfDirectoryRecord = 0;    // 00h 1  Length of Directory Record (33+LEN_FI+pad+LEN_SU) (0=Pad)
        U8  ExtendedAttributeRecordLength;  // 01h 1  Extended Attribute Record Length (usually 00h)
        U32 DataLogicalBlockNumberLE;       // 02h 4  Data Logical Block Number (32-bit little-endian)
        U32 DataLogicalBlockNumberBE;       // 06h 4  Data Logical Block Number (32-bit big-endian)
        U32 DataSizeLE;                     // 0Ah 4  Data Size in Bytes (32-bit little-endian)
        U32 DataSizeBE;                     // 0Eh 4  Data Size in Bytes (32-bit big-endian)
        Array<U8, 7> RecordingTimestamp;    // 12h 7  Recording Timestamp (yy-1900,mm,dd,hh,mm,ss,timezone)
        U8  FileFlags;                      // 19h 1  File Flags 8 bits (usually 00h=File, or 02h=Directory)
        U8  FileUnitSize;                   // 1Ah 1  File Unit Size (usually 00h)
        U8  InterleaveGapSize;              // 1Bh 1  Interleave Gap Size (usually 00h)
        U16 VolumeSequenceNumberLE;         // 1Ch 2  Volume Sequence Number (16-bit little-endian)
        U16 VolumeSequenceNumberBE;         // 1Eh 2  Volume Sequence Number (16-bit big-endian)
        U8  LengthOfName;                   // 20h 1  Length of Name (LEN_FI)
        String Name;                        // 21h 1  Name
        Vector<U8> SystemUse;               // 22h 14 System Use (LEN_SU bytes) (00h-filled for PSX)
    };

    struct RootDirectoryRecord {
        U8  LengthOfDirectoryRecord;        // 00h 1  Length of Directory Record (33+LEN_FI+pad+LEN_SU) (0=Pad)
        U8  ExtendedAttributeRecordLength;  // 01h 1  Extended Attribute Record Length (usually 00h)
        U32 DataLogicalBlockNumberLE;       // 02h 4  Data Logical Block Number (32-bit little-endian)
        U32 DataLogicalBlockNumberBE;       // 06h 4  Data Logical Block Number (32-bit big-endian)
        U32 DataSizeLE;                     // 0Ah 4  Data Size in Bytes (32-bit little-endian)
        U32 DataSizeBE;                     // 0Eh 4  Data Size in Bytes (32-bit big-endian)
        Array<U8, 7> RecordingTimestamp;    // 12h 7  Recording Timestamp (yy-1900,mm,dd,hh,mm,ss,timezone)
        U8  FileFlags;                      // 19h 1  File Flags 8 bits (usually 00h=File, or 02h=Directory)
        U8  FileUnitSize;                   // 1Ah 1  File Unit Size (usually 00h)
        U8  InterleaveGapSize;              // 1Bh 1  Interleave Gap Size (usually 00h)
        U16 VolumeSequenceNumberLE;         // 1Ch 2  Volume Sequence Number (16-bit little-endian)
        U16 VolumeSequenceNumberBE;         // 1Eh 2  Volume Sequence Number (16-bit big-endian)
        U8  LengthOfName;                   // 20h 1  Length of Name (LEN_FI)
        U8 SystemUse;                       // 22h 14 System Use (LEN_SU bytes) (00h-filled for PSX)
    };

    struct PrimaryVolumeDescriptor {
        U8   VolumeDescriptorType;           // 000h 1    Volume Descriptor Type (01h = Primary Volume Descriptor)
        Array<Char, 5> StandardIdentifier;   // 001h 5    Standard Identifier ("CD001")
        U8   VolumeDescriptorVersion;        // 006h 1    Volume Descriptor Version (01h = Standard)
        U8   Reserved1;                      // 007h 1    Reserved (00h)
        Array<Char, 32> SystemIdentifier;    // 008h 32   System Identifier (a-characters) ("PLAYSTATION")
        Array<Char, 32> VolumeIdentifier;    // 028h 32   Volume Identifier (d-characters) (max 8 chars for PSX?)
        Array<U8, 8> Reserved2;              // 048h 8    Reserved (00h)
        U32  VolumeSpaceSizeLE;              // 050h 4    Volume Space Size (32-bit little-endian)
        U32  VolumeSpaceSizeBE;              // 054h 4    Volume Space Size (32-bit big-endian)
        Array<U8, 32> Reserved3;             // 058h 32   Reserved (00h)
        U16  VolumeSetSizeLE;                // 078h 2    Volume Set Size (16-bit little-endian)
        U16  VolumeSetSizeBE;                // 07Ah 2    Volume Set Size (16-bit big-endian)
        U16  VolumeSequenceNumberLE;         // 07Ch 2    Volume Sequence Number (16-bit little-endian)
        U16  VolumeSequenceNumberBE;         // 07Eh 2    Volume Sequence Number (16-bit big-endian)
        U16  LogicalBlockSizeLE;             // 080h 2    Logical Block Size in Bytes (16-bit little-endian)
        U16  LogicalBlockSizeBE;             // 082h 2    Logical Block Size in Bytes (16-bit big-endian)
        U32  PathTableSizeLE;                // 084h 4    Path Table Size in Bytes (32-bit little-endian)
        U32  PathTableSizeBE;                // 088h 4    Path Table Size in Bytes (32-bit big-endian)
        U32  PathTable1BlockNumberLE;        // 08Ch 4    Path Table 1 Block Number (32-bit little-endian)
        U32  PathTable2BlockNumberLE;        // 090h 4    Path Table 2 Block Number (32-bit little-endian)
        U32  PathTable3BlockNumberBE;        // 094h 4    Path Table 3 Block Number (32-bit big-endian)
        U32  PathTable4BlockNumberBE;        // 098h 4    Path Table 4 Block Number (32-bit big-endian)
        RootDirectoryRecord RootDirectoryRecord;   // 09Ch 34   Root Directory Record (see next chapter)
        Array<Char, 128> VolumeSetIdentifier;// 0BEh 128  Volume Set Identifier (d-characters) (usually empty)
        Array<Char, 128> PublisherIdentifier;// 13Eh 128  Publisher Identifier (a-characters) (company name)
        Array<Char, 128> DataPreparerIdentifier; // 1BEh 128 Data Preparer Identifier (a-characters) (empty or other)
        Array<Char, 128> ApplicationIdentifier;  // 23Eh 128 Application Identifier (a-characters) ("PLAYSTATION")
        Array<Char, 37> CopyrightFilename;       // 2BEh 37  Copyright Filename ("FILENAME.EXT;VER") (empty or text)
        Array<Char, 37> AbstractFilename;        // 2E3h 37  Abstract Filename ("FILENAME.EXT;VER") (empty)
        Array<Char, 37> BibliographicFilename;   // 308h 37  Bibliographic Filename ("FILENAME.EXT;VER") (empty)
        Array<Char, 17> VolumeCreationTimestamp; // 32Dh 17  Volume Creation Timestamp ("YYYYMMDDHHMMSSFF",timezone)
        Array<Char, 17> VolumeModificationTimestamp; // 33Eh 17 Volume Modification Timestamp ("0000000000000000",00h)
        Array<Char, 17> VolumeExpirationTimestamp;  // 34Fh 17 Volume Expiration Timestamp ("0000000000000000",00h)
        Array<Char, 17> VolumeEffectiveTimestamp;  // 360h 17 Volume Effective Timestamp ("0000000000000000",00h)
        U8   FileStructureVersion;           // 371h 1    File Structure Version (01h = Standard)
        U8   Reserved4;                      // 372h 1    Reserved for future (00h-filled)
        Array<U8, 141> ApplicationUseArea;   // 373h 141  Application Use Area (00h-filled for PSX and VCD)
        Array<Char, 8> CDXAIdentifyingSignature; // 400h 8  CD-XA Identifying Signature ("CD-XA001" for PSX and VCD)
        U16  CDXAFlags;                      // 408h 2    CD-XA Flags (unknown purpose) (00h-filled for PSX and VCD)
        Array<U8, 8> CDXAStartupDirectory;   // 40Ah 8    CD-XA Startup Directory (00h-filled for PSX and VCD)
        Array<U8, 8> CDXAReserved;           // 412h 8    CD-XA Reserved (00h-filled for PSX and VCD)
        Array<U8, 345> ApplicationUseArea2;  // 41Ah 345  Application Use Area (00h-filled for PSX and VCD)
        Array<U8, 653> Reserved5;            // 573h 653  Reserved for future (00h-filled)
    };

    struct PathTableRecord {
        U8  LengthOfDirectoryName;          // 00h 1  Length of Directory Name (LEN_DI) (01h..08h for PSX)
        U8  ExtendedAttributeRecordLength;  // 01h 1  Extended Attribute Record Length (usually 00h)
        U32 DirectoryLogicalBlockNumber;    // 02h 4  Directory Logical Block Number (32-bit little-endian or big-endian depending on the table)
        U16 ParentDirectoryNumber;          // 06h 2  Parent Directory Number (0001h and up)
        String DirectoryName;         // 08h LEN_DI  Directory Name (d-characters, d1-characters) (or 00h for Root)
    };

    struct FileSystemDirectory {
        I32 Number = 0;
        PathTableRecord PathTableRecord;
        DirectoryRecord DirectoryInfo = {};
        UnorderedMap<String, FileSystemDirectory> ChildDirectories;
        UnorderedMap<String, DirectoryRecord> Files;
    };

    using FileSystemRoot = FileSystemDirectory;

#pragma pack(pop) // Ripristina l'allineamento predefinito

	class ISO9660 {
	public:
		ISO9660(const SharedPtr<CompactDisk>& cd);
		~ISO9660() = default;

        Vector<String> ListDirectories(const StringView& path);
        const DirectoryRecord& GetDirectoryInfo(const StringView& path);

        Vector<String> ListFiles(const StringView& directoryPath);
        const DirectoryRecord& GetFileInfo(const StringView& filePath);
        void GetFileData(const StringView& filePath, Vector<U8>& data);

    private:
        void ReadPrimaryVolumeDecriptor();
        void ReadPathTable();
        void ReadDirectoryEntries(FileSystemDirectory* directory);
        void ReadFileData(const DirectoryRecord& file, Vector<U8>& data);

        FileSystemDirectory* SearchDirectory(const StringView& path);
        const DirectoryRecord& SearchFile(const StringView& filePath);

        String NormalizePath(const StringView& path);
	private:
		SharedPtr<CompactDisk> mCD;
        PrimaryVolumeDescriptor mPVD;
        FileSystemRoot mFileSystem = {};
	};

}