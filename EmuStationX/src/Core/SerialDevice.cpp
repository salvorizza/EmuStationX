#include "SerialDevice.h"

#include "Utils/LoggingSystem.h"

namespace esx {



	SerialDevice::SerialDevice(SerialDeviceType type)
		: mDeviceType(type)
	{
	}

	void SerialDevice::mosi(U8 value)
	{
		mRX.Push(value);
		if (mRX.Size == 8) {
			U8 tx = receive(mRX.Data);
			if (mDeviceType == SerialDeviceType::MemoryCard) {
				ESX_CORE_LOG_TRACE("{:02X}h\t{:02X}h", mRX.Data, tx);
			}
			mTX.Set(tx);
			mRX = {};
		}
	}

	U8 SerialDevice::miso()
	{
		return mTX.Pop();
	}

}