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
			ESX_CORE_LOG_TRACE("TX {:02x}h", mRX.Data);

			U8 tx = receive(mRX.Data);

			ESX_CORE_LOG_TRACE("RX {:02x}h", tx);

			mTX.Set(tx);
			mRX = {};
		}
	}

	U8 SerialDevice::miso()
	{
		return mTX.Pop();
	}

}