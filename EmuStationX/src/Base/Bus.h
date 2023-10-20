#pragma once

#include <cstdint>
#include <unordered_map>
#include <memory>
#include <string>
#include <optional>
#include <cassert>

namespace esx {

	struct BusRange {
		size_t Start;
		size_t End;
		size_t Mask;

		BusRange(size_t start, size_t sizeInBytes, size_t mask)
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

		virtual void write(const std::string& busName, uint32_t address, uint8_t value) = 0;
		virtual void read(const std::string& busName, uint32_t address, uint8_t& output) = 0;

		virtual void write(const std::string& busName, uint32_t address, uint16_t value) = 0;
		virtual void read(const std::string& busName, uint32_t address,uint16_t& output) = 0;

		virtual void write(const std::string& busName, uint32_t address, uint32_t value) = 0;
		virtual void read(const std::string& busName, uint32_t address, uint32_t& output) = 0;

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
					device->write(mName, address & range->Mask, value);
				}
			}

			assert(found && "Address not found");
		}

		template<typename T>
		T read(uint32_t address) {
			T result = 0;

			bool found = false;
			for (auto& [name, device] : mDevices) {
				auto range = device->getRange(mName, address);
				if (range) {
					found = true;
					device->read(mName, address & range->Mask, result);
					break;
				}
			}

			assert(found && "Address not found");

			return result;
		}


		void connectDevice(BusDevice* device);

		const std::string& getName() const { return mName; }

	private:
		std::string mName;
		std::unordered_map<std::string,BusDevice*> mDevices;
	};


}