#pragma once

#include <cstdint>
#include <unordered_map>
#include <memory>
#include <string>
#include <optional>

namespace esx {

	struct BusRange {
		uint32_t Start;
		uint32_t End;
		uint32_t Mask;

		BusRange(uint32_t start, uint32_t end, uint32_t mask)
			:	Start(start),
				End(end),
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
		virtual uint8_t read(const std::string& busName, uint32_t address) = 0;

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
		void write(uint32_t address, uint8_t value);
		uint8_t read(uint32_t address);

		void connectDevice(BusDevice* device);

		const std::string& getName() const { return mName; }

	private:
		std::string mName;
		std::unordered_map<std::string,BusDevice*> mDevices;
	};


}