#pragma once

#include "Base/Base.h"

namespace esx {

	enum class ControllerType : U16 {
		Mouse = 0x5A12,
		NegCon = 0x5A23,
		JonamiLightgun = 0x5A31,
		DigitalPad = 0x5A41,
		AnalogStick = 0x5A53,
		NamcoLightgun = 0x5A63,
		AnalogPad = 0x5A73,
		Dualshock2 = 0x5A79,
		Multitap = 0x5A80,
		Keyboard = 0x5A96,
		Jogcon = 0x5AE3,
		KeyboardSticks = 0x5AE8,
		ConfigMode = 0x5AF3
	};

	enum class ControllerButton : U16 {
		Select = 1 << 0,
		L3 = 1 << 1,
		R3 = 1 << 2,
		Start = 1 << 3,
		JoypadUp = 1 << 4,
		JoypadRight = 1 << 5,
		JoypadDown = 1 << 6,
		JoypadLeft = 1 << 7,
		L2 = 1 << 8,
		R2 = 1 << 9,
		L1 = 1 << 10,
		R1 = 1 << 11,
		Triangle = 1 << 12,
		Circle = 1 << 13,
		Cross = 1 << 14,
		Square = 1 << 15
	};

	enum CommunicationPhase {
		IDLo,
		IDHi,
		Data1,
		Data2,
		Max
	};

	using ControllerState = U16;

	class SIO;
	struct ShiftRegister;

	class Controller {
	public:
		Controller(ControllerType type);
		~Controller() = default;

		void mosi(U8 value);
		U8 miso();

		inline ControllerType getType() const { return mType; }
		inline ControllerState getState() const { return mState; }

		inline void setButtonState(ControllerButton button, BIT pressed) { if (pressed) pressButton(button); else releaseButton(button); }
		inline void releaseButton(ControllerButton button) { mState |= (U16)button; }
		inline void pressButton(ControllerButton button) { mState &= ~((U16)button); }

		inline void setMaster(const SharedPtr<SIO>& master) { mMaster = master; }
	private:
		ControllerType mType;
		ControllerState mState;
		SharedPtr<SIO> mMaster;

		ShiftRegister mRX, mTX;

		CommunicationPhase mPhase;
	};

}