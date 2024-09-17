#include "CDROMDrive.h"

namespace esx::platform {



    CDROMDrive::CDROMDrive(const StringView& devicePath)
        :   mDevicePath(devicePath),
            mDeviceHandle(NULL),
            mTOC({0})
    {
        String filePath = String("\\\\.\\") + String(devicePath);
        filePath.erase(filePath.find_last_of(':') + 1, filePath.size() - filePath.find_last_of(':') - 1);

        HANDLE hDevice = CreateFile(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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

    void CDROMDrive::ReadSector(U32 lbaBytes, Sector* sector,U32 numSectors)
    {
        RAW_READ_INFO rawReadInfo = { 0 };
        DWORD bytesRead = 0;

        rawReadInfo.DiskOffset.QuadPart = (lbaBytes / 2352) * 2048;
        rawReadInfo.SectorCount = numSectors;
        rawReadInfo.TrackMode = YellowMode2;

        BOOL bResult = DeviceIoControl(mDeviceHandle, IOCTL_CDROM_RAW_READ, &rawReadInfo, sizeof(RAW_READ_INFO), sector, sizeof(Sector), &bytesRead, (LPOVERLAPPED)NULL);
        if (bResult == FALSE) {
            DWORD error = GetLastError();
            ESX_CORE_LOG_ERROR("IOCTL_CDROM_RAW_READ failed with {} code", error);
            return;
        }
    }

    void CDROMDrive::ReadSubchannelQ(SubchannelQ* subchannelQ)
    {
        CDROM_SUB_Q_DATA_FORMAT subQDataFormat = { 0 };
        SUB_Q_CHANNEL_DATA subQChannelData = { 0 };
        DWORD bytesRead = 0;

        subQDataFormat.Format = IOCTL_CDROM_CURRENT_POSITION;

        BOOL bResult = DeviceIoControl(mDeviceHandle, IOCTL_CDROM_READ_Q_CHANNEL, &subQDataFormat, sizeof(CDROM_SUB_Q_DATA_FORMAT), &subQChannelData, sizeof(SUB_Q_CHANNEL_DATA), &bytesRead, (LPOVERLAPPED)NULL);
        if (bResult == FALSE) {
            DWORD error = GetLastError();
            ESX_CORE_LOG_ERROR("IOCTL_CDROM_READ_Q_CHANNEL failed with {} code", error);
            return;
        }

        
        subchannelQ->Track = subQChannelData.CurrentPosition.TrackNumber;
        subchannelQ->Index = subQChannelData.CurrentPosition.IndexNumber;
        subchannelQ->Absolute.Minute = subQChannelData.CurrentPosition.AbsoluteAddress[0];
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