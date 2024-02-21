#include "Bus.h"

#include <cassert>

namespace esx {

	

	Bus::Bus(const StringView& name)
		: mName(name)
	{
	}

	void Bus::writeLine(const StringView& lineName, BIT value)
	{
		for (const auto& [name, device] : mDevices) {
			device->writeLine(mName, lineName, value);
		}
	}

	void Bus::connectDevice(const SharedPtr<BusDevice>& device) {
		mDevices[device->getName()] = device;
	}

	void Bus::addRange(const StringView& deviceName, BusRange range)
	{
		mRanges[range.End] = std::make_pair(range, mDevices.at(deviceName));
	}

	void BusDevice::connectToBus(const SharedPtr<Bus>& pBus)
	{
		for (const BusRange& range : mStoredRanges) {
			pBus->addRange(mName, range);
		}
		mBusses[pBus->getName()] = pBus;
	}

	SharedPtr<Bus> BusDevice::getBus(const StringView& busName)
	{
		return mBusses[busName];
	}

	void BusDevice::addRange(const StringView& busName, U64 start, U64 sizeInBytes, U64 mask)
	{
		mStoredRanges.emplace_back(start, sizeInBytes, mask);
	}



}