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
		BusDevice(const String& name) : mName(name) {}
		virtual ~BusDevice() = default;

		virtual void writeLine(const String& busName, const String& lineName, bool value) {}

		virtual void store(const String& busName, U32 address, U32 value) { ESX_CORE_ASSERT(false, "Device {} does not implement store32", mName); }
		virtual void load(const String& busName, U32 address, U32& output) { ESX_CORE_ASSERT(false, "Device {} does not implement load32", mName); }

		virtual void store(const String& busName, U32 address, U16 value) { ESX_CORE_ASSERT(false, "Device {} does not implement store16", mName); }
		virtual void load(const String& busName, U32 address, U16& output) { ESX_CORE_ASSERT(false, "Device {} does not implement load16", mName); }

		virtual void store(const String& busName, U32 address, U8 value) { ESX_CORE_ASSERT(false, "Device {} does not implement store8", mName); }
		virtual void load(const String& busName, U32 address, U8& output) { ESX_CORE_ASSERT(false, "Device {} does not implement load8", mName); }

		std::optional<BusRange> getRange(const String& busName, U32 address);

		const String& getName() const { return mName; }

		void connectToBus(Bus* pBus);
		Bus* getBus(const String& busName);

	protected:
		template<typename... T>
		void addRange(const String& busName, T&&... args) {
			mRanges[busName].emplace_back(std::forward<T>(args)...);
		}

	protected:
		UnorderedMap<String, Bus*> mBusses;
		UnorderedMap<String, Vector<BusRange>> mRanges;

		String mName;
	};


	class Bus {
	public:
		Bus(const String& name);
		~Bus();

		void writeLine(const String& lineName, bool value);

		template<typename T>
		void store(U32 address, T value) {
			bool found = false;
			for (auto& [name, device] : mDevices) {
				auto range = device->getRange(mName, address);
				if (range) {
					found = true;
					device->store(mName, address & range->Mask, value);
				}
			}

			ESX_ASSERT(found, "Writing Address 0x{:08X}: not found", address);
		}

		template<typename T>
		T load(U32 address) {
			T result = 0;

			bool found = false;
			for (auto& [name, device] : mDevices) {
				auto range = device->getRange(mName, address);
				if (range) {
					found = true;
					device->load(mName, address & range->Mask, result);
					break;
				}
			}

			ESX_ASSERT(found, "Reading Address 0x{:08X}: not found", address);

			return result;
		}

		void connectDevice(BusDevice* device);

		template<typename T>
		T* getDevice(const String& name) {
			return dynamic_cast<T*>(mDevices.at(name));
		}

		const String& getName() const { return mName; }

	private:
		String mName;
		UnorderedMap<String,BusDevice*> mDevices;
	};


}