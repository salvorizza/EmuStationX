#pragma once

#include "Base/Base.h"

#include "SerialDevice.h"

namespace esx {

	enum class ControllerType : U16 {
		Mouse = 0x5A12,
		NegCon = 0x5A23,
		KonamiLightgun = 0x5A31,
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

	enum class CommunicationPhase {
		Addressing,
		Command,
		GetVariableResponseBii,
		GetVariableResponseAii,
		Data,
		NotSelected
	};

	enum class ControllerCommand : U8 {
		None = 0x00,
		GetSetButtonAttr = 0x40, //"@" Unused, or Dualshock2: Get / Set ButtonAttr ?
		GetReplyCapabilities = 0x41, //"A" Unused, or Dualshock2 : Get Reply Capabilities
		Read = 0x42, //"B" Read Buttons AND analog inputs(even when in digital mode)
		EnterExitConfigurationMode = 0x43, //"C" Enter / Exit Configuration Mode(stay config, or exit)
		SetLEDState = 0x44, //"D" Set LED State(analog mode on / off)
		GetLEDState = 0x45, //"E" Get LED State(and Type / constants)
		GetVariableResponseA = 0x46, //"F" Get Variable Response A(depending on incoming bit)
		GetWhateverValues = 0x47, //"G" Get whatever values(response HiZ F3h 5Ah 00h 00h 02h 00h 01h 00h)
		Unknown1 = 0x48, //"H" Unknown(response HiZ F3h 5Ah 00h 00h 00h 00h 01h 00h)
		Unused1 = 0x49, //"I" Unused
		Unused2 = 0x4A, //"J" Unused
		Unused3 = 0x4B, //"K" Unused
		GetVariableResponseB = 0x4C, //"L" Get Variable Response B(depending on incoming bit)
		GetSetRumbleProtocol = 0x4D, //"M" Get / Set RumbleProtocol
		Unused4 = 0x4E, //"N" Unused
		Unused5 = 0x4F, //"O" Unused, or Dualshock2: Set ReplyProtocol
	};

	enum class ControllerMode : U8 {
		Normal,
		Configuration
	};

	using ControllerState = U16;

	class SIO;
	struct ShiftRegister;
	class Controller;

	using UpdateFunction = Function<void(Controller*)>;


	class Controller : public SerialDevice {
	public:
		Controller(ControllerType type);
		virtual ~Controller() = default;

		U8 receive(U8 value) override;
		void cs() override;

		inline ControllerType getType() const { return mType; }
		inline ControllerState getState() const { return mState; }

		inline void setButtonState(ControllerButton button, BIT pressed) { if (pressed) pressButton(button); else releaseButton(button); }
		inline void setUpdateFunction(const UpdateFunction& updateFunction) { mUpdateFunction = updateFunction; }
		inline void releaseButton(ControllerButton button) { mState |= (U16)button; }
		inline void pressButton(ControllerButton button) { mState &= ~((U16)button); }
	private:
		ControllerType mType;
		ControllerState mState;
		CommunicationPhase mPhase;
		ControllerCommand mCurrentCommand;
		ControllerMode mMode;
		Queue<U8> mCommandResponse;
		UpdateFunction mUpdateFunction;
	};

}