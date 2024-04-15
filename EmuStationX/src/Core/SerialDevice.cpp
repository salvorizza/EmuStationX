#include "SerialDevice.h"

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
			mTX.Set(tx);
			mRX = {};
		}
	}

	U8 SerialDevice::miso()
	{
		return mTX.Pop();
	}

}