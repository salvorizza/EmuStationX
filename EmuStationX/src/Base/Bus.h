#pragma once

#include "Base/Base.h"
#include "Base/Assert.h"

#include <optional>

namespace esx {

	struct BusRange {
		U64 Start;
		U64 End;
		U64 Mask;

		BusRange(U64 start, U64 sizeInBytes, U64 mask)
			:	Start(start),
				End(start + sizeInBytes),
				Mask(mask)
		{}
	};

	typedef class Bus;

	class BusDevice {
	public:
		BusDevice(const std::string& name) : mName(name) {}
		virtual ~BusDevice() = default;

		virtual void writeLine(const std::string& busName, const std::string& lineName, bool value) {}

		virtual void write(const std::string& busName, uint32_t address, uint32_t value, size_t valueSize) { assert(false && "Device does not implement write32"); }
		virtual uint32_t read(const std::string& busName, uint32_t address, size_t outputSize) { assert(false && "Device does not implement read32"); return 0; }

		std::optional<BusRange> getRange(const std::string& busName, uint32_t address);

		const std::string& getName() const { return mName; }

		void connectToBus(Bus* pBus);
		Bus* getBus(const std::string& busName);

	protected:
		template<typename... T>
		void addRange(const std::string& busName, T&&... args) {
			mRanges[busName].emplace_back(std::forward<T>(args)...);
		}

	protected:
		std::unordered_map<std::string, Bus*> mBusses;
		std::unordered_map<std::string, std::vector<BusRange>> mRanges;

		std::string mName;
	};


	class Bus {
	public:
		Bus(const std::string& name);
		~Bus();

		void writeLine(const std::string& lineName, bool value);

		template<typename T>
		void write(uint32_t address, T value) {
			bool found = false;
			for (auto& [name, device] : mDevices) {
				auto range = device->getRange(mName, address);
				if (range) {
					found = true;
					device->write(mName, address & range->Mask, value, sizeof(T));
				}
			}

			ESX_ASSERT(found, "Writing Address 0x{:8X}: not found", address);
		}

		template<typename T>
		uint32_t read(uint32_t address) {
			uint32_t result = 0;

			bool found = false;
			for (auto& [name, device] : mDevices) {
				auto range = device->getRange(mName, address);
				if (range) {
					found = true;
					result = device->read(mName, address & range->Mask, sizeof(T));
					break;
				}
			}

			ESX_ASSERT(found, "Reading Address 0x{:8X}: not found", address);

			return result;
		}

		void connectDevice(BusDevice* device);

		const std::string& getName() const { return mName; }

	private:
		std::string mName;
		std::unordered_map<std::string,BusDevice*> mDevices;
	};


}