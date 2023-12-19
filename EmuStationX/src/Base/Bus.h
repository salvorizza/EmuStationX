#pragma once

#include "Base/Base.h"
#include "Base/Assert.h"

#include <optional>

namespace esx {

	struct BusRange {
		U64 Start;
		U64 End;
		U64 Mask;

		BusRange()
			: Start(0),
			End(0),
			Mask(0)
		{}

		BusRange(U64 start, U64 sizeInBytes, U64 mask)
			:	Start(start),
				End(start + sizeInBytes),
				Mask(mask)
		{}
	};

	typedef class Bus;

	class BusDevice {
	public:
		BusDevice(const StringView& name) : mName(name) {}
		virtual ~BusDevice() = default;

		virtual void writeLine(const StringView& busName, const StringView& lineName, BIT value) {}

		virtual void store(const StringView& busName, U32 address, U32 value) { ESX_CORE_ASSERT(ESX_FALSE, "Device {} does not implement store32", mName); }
		virtual void load(const StringView& busName, U32 address, U32& output) { ESX_CORE_ASSERT(ESX_FALSE, "Device {} does not implement load32", mName); }

		virtual void store(const StringView& busName, U32 address, U16 value) { ESX_CORE_ASSERT(ESX_FALSE, "Device {} does not implement store16", mName); }
		virtual void load(const StringView& busName, U32 address, U16& output) { ESX_CORE_ASSERT(ESX_FALSE, "Device {} does not implement load16", mName); }

		virtual void store(const StringView& busName, U32 address, U8 value) { ESX_CORE_ASSERT(ESX_FALSE, "Device {} does not implement store8", mName); }
		virtual void load(const StringView& busName, U32 address, U8& output) { ESX_CORE_ASSERT(ESX_FALSE, "Device {} does not implement load8", mName); }

		const StringView& getName() const { return mName; }

		void connectToBus(Bus* pBus);
		Bus* getBus(const StringView& busName);

	protected:
		void addRange(const StringView& busName, U64 start, U64 sizeInBytes, U64 mask);
	protected:
		UnorderedMap<StringView, Bus*> mBusses;
		Vector<BusRange> mStoredRanges;

		StringView mName;
	};


	class Bus {
	public:
		Bus(const StringView& name);
		~Bus();

		void writeLine(const StringView& lineName, BIT value);

		template<typename T>
		void store(U32 address, T value) {
			auto it = mRanges.upper_bound(address);
			if (it != mRanges.end()) {
				auto [busRange, device] = it->second;
				if (address >= busRange.Start && address < busRange.End) {
					device->store(mName, address & busRange.Mask, value);
				} else {
					ESX_ASSERT(ESX_FALSE, "Writing Address 0x{:08x}: not found", address);
				}
			}
			else {
				ESX_ASSERT(ESX_FALSE, "Writing Address 0x{:08x}: not found", address);
			}
		}

		template<typename T>
		T load(U32 address) {
			T result = 0;

			auto it = mRanges.upper_bound(address);
			if (it != mRanges.end()) {
				auto [busRange, device] = it->second;

				if (address >= busRange.Start && address < busRange.End) {
					device->load(mName, address & busRange.Mask, result);
				} else {
					ESX_ASSERT(ESX_FALSE, "Reading Address 0x{:08x}: not found", address);
				}
			} else {
				ESX_ASSERT(ESX_FALSE, "Reading Address 0x{:08x}: not found", address);
			}

			return result;
		}

		void connectDevice(BusDevice* device);

		template<typename T>
		T* getDevice(const StringView& name) {
			return dynamic_cast<T*>(mDevices.at(name));
		}

		const StringView& getName() const { return mName; }

		void addRange(BusDevice* device, BusRange range);

	private:
		StringView mName;
		UnorderedMap<StringView,BusDevice*> mDevices;
		Map<U64, Pair<BusRange,BusDevice*>> mRanges;
	};


}