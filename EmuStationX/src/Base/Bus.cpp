#include "Bus.h"

#include <cassert>

namespace esx {

	

	Bus::Bus(const std::string& name)
		: mName(name)
	{
	}

	Bus::~Bus()
	{
	}

	void Bus::writeLine(const std::string& lineName, bool value)
	{
		for (auto& [name, device] : mDevices) {
			device->writeLine(mName, lineName, value);
		}
	}

	void Bus::connectDevice(BusDevice* device) {
		mDevices[device->getName()] = device;
		device->connectToBus(this);
	}


	std::optional<BusRange> BusDevice::getRange(const std::string& busName, uint32_t address)
	{
		auto it = std::find_if(mRanges[busName].begin(), mRanges[busName].end(), [address](const BusRange& t) -> bool {
			return address >= t.Start  && address < t.End;
		});

		if (it == mRanges[busName].end())
			return {};

		return *it;
	}

	void BusDevice::connectToBus(Bus* pBus)
	{
		mBusses[pBus->getName()] = pBus;
	}

	Bus* BusDevice::getBus(const std::string& busName)
	{
		return mBusses[busName];
	}



}