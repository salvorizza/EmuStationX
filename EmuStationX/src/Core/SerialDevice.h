#pragma once

#include "Base/Base.h"

namespace esx {

	enum class SerialDeviceType {
		Controller,
		MemoryCard
	};

	class SIO;

	class SerialDevice {
	public:
		SerialDevice(SerialDeviceType type);
		~SerialDevice() = default;

		virtual U8 receive(U8 data) = 0;
		virtual void cs() = 0;

		void mosi(U8 value);
		U8 miso();

		inline SerialDeviceType getDeviceType() const { return mDeviceType; }
		inline void setMaster(const SharedPtr<SIO>& master) { mMaster = master; }

		inline BIT isSelected() const { return mSelected; }

	protected:
		SharedPtr<SIO> mMaster;
		ShiftRegister mRX, mTX;
		BIT mSelected = ESX_FALSE;
	private:
		SerialDeviceType mDeviceType;
	};

}