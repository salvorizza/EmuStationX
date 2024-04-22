#include "SIO.h"

#include "Controller.h"

#include "InterruptControl.h"
#include "R3000.h"

namespace esx {

	SIO::SIO(U8 id)
		:	BusDevice(id == 0 ? "SIO0" : "SIO1"),
			mID(id)
	{
		addRange(ESX_TEXT("Root"), 0x1F801040 + mID * 0x10, BYTE(16), 0xFFFFFFFF);
		mRX.Data.fill(0x00);
	}

	SIO::~SIO()
	{
	}

	void SIO::clock()
	{
		if (mStatRegister.BaudrateTimer > 0) {
			mStatRegister.BaudrateTimer--;
			if (mStatRegister.BaudrateTimer == 0) {
				if (mSerialClock == ESX_TRUE) {
					fallingEdge();
				} else {
					risingEdge();
				}

				mSerialClock = !mSerialClock;
				
				reloadBaudTimer();
			}
		}
	}

	void SIO::fallingEdge()
	{
		if (mTXShift.Size != 0) {
			if (mStatRegister.TXFifoNotFull == ESX_FALSE) {
				mStatRegister.TXFifoNotFull = ESX_TRUE;
				//ESX_CORE_LOG_TRACE("TX {:02X}h", mTX);
			}

			U8 value = mTXShift.Pop();

			for (auto& device : mPorts[mControlRegister.PortSelect]) {
				if (device) {
					device->mosi(value);
					if (device->isSelected()) {
						break;
					}
				}
			}

			if (mTXShift.Size == 0) {
				mRXShift = {};
				mStatRegister.TXIdle = ESX_TRUE;
			}
		}
	}

	void SIO::risingEdge()
	{
		if (canReceiveData()) {
			U8 value = 1;

			for (auto& device : mPorts[mControlRegister.PortSelect]) {
				if (device) {
					U8 misoValue = device->miso();
					if (device->isSelected()) {
						value = misoValue;
						break;
					}
				}
			}

			mRXShift.Push(value);

			if (mRXShift.Size == 8) {
				U8 rx = mRXShift.Data;
				mRXShift = {};

				mRX.Push(rx);

				//ESX_CORE_LOG_TRACE("RX {:02X}h", rx);

				mStatRegister.RXFifoNotEmpty = ESX_TRUE;
				if (mControlRegister.RXEnable) mControlRegister.RXEnable = ESX_FALSE;
			}

		}
	}

	void SIO::dsr(U64 delay)
	{
		//mStatRegister.DSRInputLevel = ESX_TRUE;

		if (mControlRegister.DSRInterruptEnable && mControlRegister.TXEnable) {
			getBus("Root")->getDevice<InterruptControl>("InterruptControl")->requestInterrupt(InterruptType::ControllerAndMemoryCard, mStatRegister.InterruptRequest, ESX_TRUE, delay);
			mStatRegister.InterruptRequest = ESX_TRUE;
		}
	}

	void SIO::store(const StringView& busName, U32 address, U16 value)
	{
		if (mID == 1) {
			address -= 0x10;
		}

		switch (address) {
			case 0x1F801040:
			case 0x1F801042: {
				setDataRegister(value);
				break;
			}

			case 0x1F801048: {
				setModeRegister(value);
				break;
			}
			case 0x1F80104A: {
				setControlRegister(value);
				break;
			}
			case 0x1F80104C: {
				if (mID == 1) {
					setMiscRegister(value);
				}
				break;
			}
			case 0x1F80104E: {
				setBaudRegister(value);
				break;
			}
		}
	}

	void SIO::load(const StringView& busName, U32 address, U16& output)
	{
		if (mID == 1) {
			address -= 0x10;
		}

		switch (address) {
			case 0x1F801040:
			case 0x1F801042: {
				output = getDataRegister(16);
				break;
			}

			case 0x1F801044: {
				output = getStatRegister();
				break;
			}

			case 0x1F801048: {
				output = getModeRegister();
				break;
			}

			case 0x1F80104A: {
				output = getControlRegister();
				break;
			}

			case 0x1F80104C: {
				if (mID == 1) {
					output = getMiscRegister();
				} else {
					output = 0;
				}
				break;
			}

			case 0x1F80104E: {
				output = getBaudRegister();
				break;
			}
		}
	}

	void SIO::store(const StringView& busName, U32 address, U8 value)
	{
		if (mID == 1) {
			address -= 0x10;
		}

		switch (address) {
			case 0x1F801040: {
				setDataRegister(value);
				break;
			}
		}
	}

	void SIO::load(const StringView& busName, U32 address, U8& output)
	{
		if (mID == 1) {
			address -= 0x10;
		}

		switch (address) {
			case 0x1F801040: {
				output = getDataRegister(8);
				break;
			}
		}
	}

	void SIO::setDataRegister(U32 value)
	{
		mTX = (value & 0xFF);

		if (mStatRegister.TXFifoNotFull == ESX_FALSE) mTXShift.Set(value);
		if (canTransferStart()) {
			mTXShift.Set(mTX);
			mStatRegister.TXFifoNotFull = ESX_FALSE;
			mStatRegister.TXIdle = ESX_FALSE;
		}

		mLatchedTXEN = mControlRegister.TXEnable;
	}

	U32 SIO::getDataRegister(U8 dataAccess)
	{
		U32 result = 0;

		result |= (mRX.Data[(mRX.ReadP + 0) % mRX.Size()] << 0);
		result |= (mRX.Data[(mRX.ReadP + 1) % mRX.Size()] << 8);
		result |= (mRX.Data[(mRX.ReadP + 2) % mRX.Size()] << 16);
		result |= (mRX.Data[(mRX.ReadP + 3) % mRX.Size()] << 24);

		if (dataAccess == 32) {
			for (I32 i = 0; i < 4; i++) {
				if (!mRX.Empty()) mRX.Pop();
			}
		}
		else {
			if(!mRX.Empty()) mRX.Pop();
		}

		if (mRX.Empty()) {
			mStatRegister.RXFifoNotEmpty = ESX_FALSE;
		}

		return result;
	}

	U32 SIO::getStatRegister()
	{
		U32 result = 0;

		result |= (mStatRegister.TXFifoNotFull << 0);
		result |= (mStatRegister.RXFifoNotEmpty << 1);
		result |= (mStatRegister.TXIdle << 2);
		result |= (mStatRegister.RXParityError << 3);
		if (mID == 1) {
			result |= (mStatRegister.RXFifoOverrun << 4);
			result |= (mStatRegister.RXBadStopBit << 5);
			result |= (mStatRegister.RXInputLevel << 6);
		}
		result |= (mStatRegister.DSRInputLevel << 7);
		if (mID == 1) {
			result |= (mStatRegister.CTSInputLevel << 8);
		}
		result |= (mStatRegister.InterruptRequest << 9);
		result |= (mStatRegister.Unknown << 10);
		result |= (mStatRegister.BaudrateTimer << 11);

		return result;
	}

	void SIO::setModeRegister(U16 value)
	{
		mModeRegister.BaudrateReloadFactor = (BaudrateReloadFactor)((value >> 0) & 0x3);
		mModeRegister.CharacterLength = (CharacterLength)((value >> 2) & 0x3);
		mModeRegister.ParityEnable = ((value >> 4) & 0x1);
		mModeRegister.ParityType = (ParityType)((value >> 5) & 0x1);
		if (mID == 1) {
			mModeRegister.StopBitLength = (StopBitLength)((value >> 6) & 0x3);
		}
		if (mID == 0) {
			mModeRegister.ClockPolarity = (ClockPolarity)((value >> 8) & 0x1);
		}
	}

	U16 SIO::getModeRegister()
	{
		U16 result = 0;

		result |= ((U16)mModeRegister.BaudrateReloadFactor << 0);
		result |= ((U16)mModeRegister.CharacterLength << 2);
		result |= (mModeRegister.ParityEnable << 4);
		result |= ((U16)mModeRegister.ParityType << 5);
		if (mID == 1) {
			result |= ((U16)mModeRegister.StopBitLength << 6);
		}
		if (mID == 0) {
			result |= ((U16)mModeRegister.ClockPolarity << 8);
		}

		return result;
	}

	void SIO::setControlRegister(U16 value)
	{
		SIOControlRegister prevControlRegister = mControlRegister;
		BIT wasReady = canTransferStart();

		mControlRegister.TXEnable = ((value >> 0) & 0x1);
		mControlRegister.DTROutputLevel = ((value >> 1) & 0x1);
		mControlRegister.RXEnable = ((value >> 2) & 0x1);
		mControlRegister.TXOutputLevel = ((value >> 3) & 0x1);
		mControlRegister.Acknowledge = ((value >> 4) & 0x1);
		mControlRegister.RTSOutputLevel = ((value >> 5) & 0x1);
		mControlRegister.Reset = ((value >> 6) & 0x1);
		mControlRegister.Unknown = ((value >> 7) & 0x1);
		mControlRegister.InterruptMode = (InterruptMode)((value >> 8) & 0x3);
		mControlRegister.TXInterruptEnable = ((value >> 10) & 0x1);
		mControlRegister.RXInterruptEnable = ((value >> 11) & 0x1);
		mControlRegister.DSRInterruptEnable = ((value >> 12) & 0x1);
		mControlRegister.PortSelect = ((value >> 13) & 0x1);

		BIT selected = !prevControlRegister.DTROutputLevel && mControlRegister.DTROutputLevel;
		BIT deselected = prevControlRegister.DTROutputLevel && !mControlRegister.DTROutputLevel;
		BIT portSwitch = prevControlRegister.PortSelect && !mControlRegister.PortSelect;

		if (deselected || portSwitch) {
			ESX_CORE_LOG_TRACE("/CS Assert {}", mControlRegister.PortSelect);

			for (auto& device : mPorts[SerialPort::Port1]) {
				if (device) {
					device->cs();
				}
			}
			for (auto& device : mPorts[SerialPort::Port2]) {
				if (device) {
					device->cs();
				}
			}
		}

		if (mControlRegister.Acknowledge) {
			mStatRegister.RXParityError = ESX_FALSE;
			mStatRegister.RXFifoOverrun = ESX_FALSE;
			mStatRegister.RXBadStopBit = ESX_FALSE;
			mStatRegister.InterruptRequest = ESX_FALSE;
		}

		if (mControlRegister.Reset) {
			mRX.Clear();
			mStatRegister.RXFifoNotEmpty = ESX_FALSE;

			for (auto& device : mPorts[SerialPort::Port1]) {
				if (device) {
					device->cs();
				}
			}
			for (auto& device : mPorts[SerialPort::Port2]) {
				if (device) {
					device->cs();
				}
			}

			mStatRegister.TXFifoNotFull = ESX_TRUE;
			mStatRegister.TXIdle = ESX_TRUE;

			//mModeRegister = {};
			//mControlRegister = {};
			//mBaudRegister = {};

			//mTX = 0xFF;

			mControlRegister.Reset = ESX_FALSE;
		}

		if (mControlRegister.RXEnable == ESX_FALSE && mID == 1) {
			mRX.Clear();
		}

		if (wasReady == ESX_FALSE && canTransferStart()) {
			mTXShift.Set(mTX);
			mStatRegister.TXFifoNotFull = ESX_FALSE;
			mStatRegister.TXIdle = ESX_FALSE;
		}
	}

	U16 SIO::getControlRegister()
	{
		U16 result = 0;

		result |= ((U16)mControlRegister.TXEnable << 0);
		result |= ((U16)mControlRegister.DTROutputLevel << 1);
		result |= ((U16)mControlRegister.RXEnable << 2);
		result |= ((U16)mControlRegister.TXOutputLevel << 3);
		result |= ((U16)mControlRegister.RTSOutputLevel << 5);
		result |= ((U16)mControlRegister.Unknown << 7);
		result |= ((U16)mControlRegister.InterruptMode << 8);
		result |= ((U16)mControlRegister.TXInterruptEnable << 10);
		result |= ((U16)mControlRegister.RXInterruptEnable << 11);
		result |= ((U16)mControlRegister.DSRInterruptEnable << 12);
		result |= ((U16)mControlRegister.PortSelect << 13);


		return result;
	}

	void SIO::setMiscRegister(U16 value)
	{
		mMiscRegister.InternalRegister = value;
	}

	U16 SIO::getMiscRegister()
	{
		return mMiscRegister.InternalRegister;
	}

	void SIO::setBaudRegister(U16 value)
	{
		mBaudRegister.ReloadValue = value;
		reloadBaudTimer();
	}

	U16 SIO::getBaudRegister()
	{
		return mBaudRegister.ReloadValue;
	}

	void SIO::reloadBaudTimer()
	{
		U8 baudFactor = (U8)mModeRegister.BaudrateReloadFactor;
		if (mID == 0) {
			baudFactor = (U8)BaudrateReloadFactor::MUL1;
		}

		mStatRegister.BaudrateTimer = (mBaudRegister.ReloadValue * baudFactor) / 2;
	}

	BIT SIO::canTransferStart()
	{
		BIT CTS = mID == 1 ? mStatRegister.CTSInputLevel : ESX_TRUE;
		BIT TXEN = mControlRegister.TXEnable || mLatchedTXEN;
		return TXEN && CTS && mStatRegister.TXIdle;
	}

	BIT SIO::canReceiveData()
	{
		if (mControlRegister.RXEnable) {
			return ESX_TRUE;
		} else {
			return mControlRegister.DTROutputLevel;
		}
	}

}