#include "Bus.h"

#include <cassert>

namespace esx {

	

	Bus::Bus(const StringView& name)
		: mName(name),
			mRanges()
	{
	}

	Bus::~Bus()
	{
	}

	void Bus::writeLine(const StringView& lineName, bool value)
	{
		for (auto& [name, device] : mDevices) {
			device->writeLine(mName, lineName, value);
		}
	}

	void Bus::connectDevice(BusDevice* device) {
		mDevices[device->getName()] = device;
		device->connectToBus(this);
	}

	void Bus::addRange(BusDevice* device, BusRange range)
	{
		mRanges[range.End] = std::make_pair(range, device);
	}

	void BusDevice::connectToBus(Bus* pBus)
	{
		for (BusRange& range : mStoredRanges) {
			pBus->addRange(this, range);
		}
		mBusses[pBus->getName()] = pBus;
	}

	Bus* BusDevice::getBus(const StringView& busName)
	{
		return mBusses[busName];
	}

	void BusDevice::addRange(const StringView& busName, U64 start, U64 sizeInBytes, U64 mask)
	{
		mStoredRanges.emplace_back(start, sizeInBytes, mask);
	}



}