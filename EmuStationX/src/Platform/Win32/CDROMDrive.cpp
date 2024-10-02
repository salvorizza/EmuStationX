#include "CDROMDrive.h"

namespace esx::platform {



    CDROMDrive::CDROMDrive(const StringView& devicePath)
        :   mDevicePath(devicePath),
            mDeviceHandle(NULL),
            mTOC({0})
    {
        String filePath = String("\\\\.\\") + String(devicePath);
        filePath.erase(filePath.find_last_of(':') + 1, filePath.size() - filePath.find_last_of(':') - 1);

        HANDLE hDevice = CreateFile(filePath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
        if (hDevice == INVALID_HANDLE_VALUE) {
            ESX_CORE_LOG_ERROR("CreateFile failed with {} code", GetLastError());
            return;
        }

        DWORD bytesRead = 0;
        CDROM_TOC toc = { 0 };
        BOOL bResult = DeviceIoControl(hDevice, IOCTL_CDROM_READ_TOC, NULL, 0, &toc, sizeof(CDROM_TOC), &bytesRead, (LPOVERLAPPED)NULL);
        if (bResult == FALSE) {
            ESX_CORE_LOG_ERROR("IOCTL_CDROM_READ_TOC failed with {} code", GetLastError());
            CloseHandle(hDevice);
            return;
        }


        mDeviceHandle = hDevice;
        mTOC = toc;
    }

    CDROMDrive::~CDROMDrive()
    {
        CloseHandle(mDeviceHandle);
    }

    void CDROMDrive::ReadSector(U32 lbaBytes, Sector* sector, SubchannelQ* subchannel, U32 numSectors)
    {
        struct read_cd {
            SCSI_PASS_THROUGH_DIRECT scsiPassThrough = { 0 };
            uint8_t dataBuffer[2352];
            uint8_t subQ[16];
            U8 senseBuffer[18];
        };

        ULONGLONG lba = (lbaBytes / 0x930);

        struct read_cd readCd = {};
        readCd.scsiPassThrough.Length = sizeof(readCd.scsiPassThrough);
        readCd.scsiPassThrough.ScsiStatus = 0;
        readCd.scsiPassThrough.CdbLength = 12;
        readCd.scsiPassThrough.DataIn = SCSI_IOCTL_DATA_IN;
        readCd.scsiPassThrough.DataTransferLength = 2352 + 16;
        readCd.scsiPassThrough.TimeOutValue = 20;
        readCd.scsiPassThrough.DataBuffer = readCd.dataBuffer;
        readCd.scsiPassThrough.SenseInfoLength = 18;
        readCd.scsiPassThrough.SenseInfoOffset = ((uint8_t*)&readCd.senseBuffer) - ((uint8_t*)&readCd);

        readCd.scsiPassThrough.Cdb[0] = 0xBE; //Operation Code
        readCd.scsiPassThrough.Cdb[1] = 0x0; //Flags

        //LBA
        readCd.scsiPassThrough.Cdb[2] = (lba >> 24) & 0xFF;
        readCd.scsiPassThrough.Cdb[3] = (lba >> 16) & 0xFF;
        readCd.scsiPassThrough.Cdb[4] = (lba >> 8) & 0xFF;
        readCd.scsiPassThrough.Cdb[5] = lba & 0xFF;

        //Transfer Length
        readCd.scsiPassThrough.Cdb[6] = 0;
        readCd.scsiPassThrough.Cdb[7] = 0;
        readCd.scsiPassThrough.Cdb[8] = 1;

        //Data Type
        readCd.scsiPassThrough.Cdb[9] = (1 << 7) | (0b11 << 5) | (1 << 4) | (1 << 3) | (0b00 << 1);

        readCd.scsiPassThrough.Cdb[10] = 0b010;
        readCd.scsiPassThrough.Cdb[11] = 0x10;

        DWORD bytesReturned;

        BOOL bResult = DeviceIoControl(mDeviceHandle, IOCTL_SCSI_PASS_THROUGH_DIRECT, &readCd, sizeof(readCd), &readCd, sizeof(readCd), &bytesReturned, NULL);
        if (bResult == FALSE) {
            DWORD error = GetLastError();
            ESX_CORE_LOG_ERROR("IOCTL_SCSI_PASS_THROUGH_DIRECT failed with {} code", error);
            return;
        }

        std::memcpy(sector, readCd.dataBuffer, sizeof(Sector));

        subchannel->Track = readCd.subQ[1];
        subchannel->Index = readCd.subQ[2];
        subchannel->Relative.Minute = readCd.subQ[3];
        subchannel->Relative.Second = readCd.subQ[4];
        subchannel->Relative.Sector = readCd.subQ[5];
        subchannel->Absolute.Minute = readCd.subQ[7];
        subchannel->Absolute.Second = readCd.subQ[8];
        subchannel->Absolute.Sector = readCd.subQ[9];
    }

    Vector<String> CDROMDrive::List() {
        Vector<String> result = {};

        DWORD dwSize = MAX_PATH;
        char szLogicalDrives[MAX_PATH] = { 0 };
        DWORD dwResult = GetLogicalDriveStrings(dwSize, szLogicalDrives);

        if (dwResult > 0 && dwResult <= MAX_PATH) {
            char* szSingleDrive = szLogicalDrives;
            while (*szSingleDrive)
            {
                UINT Type = GetDriveType(szSingleDrive);
                if (Type == DRIVE_CDROM) {
                    result.emplace_back(szSingleDrive);
                }

                // get the next drive
                szSingleDrive += strlen(szSingleDrive) + 1;
            }
        }

        return result;
	}

}