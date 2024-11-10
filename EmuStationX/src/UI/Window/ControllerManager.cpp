#include "ControllerManager.h"

#include <GLFW/glfw3.h>

namespace esx {

	UnorderedMap<ControllerID, InputController> ControllerManager::sControllers = {};

	void ControllerManager::InternalUpdate()
	{
		static Vector<U8> statesVector;
		static Vector<F32> axesVector;

		GLFWgamepadstate state;
		int stateCount = 0;
		int axesCount = 0;

		const float deadZone = 0.3;
		for (int jid = 0; jid < GLFW_JOYSTICK_LAST; jid++) {
			if (glfwJoystickPresent(jid)) {
				const unsigned char* buttonStates = NULL;
				const float* axes = NULL;

				if (glfwJoystickIsGamepad(jid) == GLFW_TRUE) {
					glfwGetGamepadState(jid, &state);

					if (state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] > deadZone) state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT] = GLFW_PRESS;
					if (state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] < -deadZone) state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT] = GLFW_PRESS;
					if (state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] > deadZone) state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN] = GLFW_PRESS;
					if (state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] < -deadZone) state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP] = GLFW_PRESS;

					buttonStates = state.buttons;
					axes = state.axes;
					stateCount = sizeof(state.buttons) / sizeof(state.buttons[0]);
					axesCount = sizeof(state.axes) / sizeof(state.axes[0]);
				}
				else {
					buttonStates = glfwGetJoystickButtons(jid, &stateCount);
					axes = glfwGetJoystickAxes(jid, &axesCount);
				}

				statesVector.clear();
				statesVector.insert(statesVector.begin(), buttonStates, buttonStates + stateCount);

				axesVector.clear();
				axesVector.insert(axesVector.begin(), axes, axes + 15);

				ControllerManager::Update(ControllerID(jid), statesVector, axesVector);
			}
		}
	}

	void ControllerManager::Connect(ControllerID cid, StringView name, BIT isGamepad)
	{
		sControllers[cid] = InputController(cid, name, isGamepad);
		sControllers[cid].CurrentStates.resize(0xE);
		sControllers[cid].PreviousStates.resize(0xE);
	}

	void ControllerManager::Disconnect(ControllerID cid)
	{
		if (sControllers.contains(cid)) {
			sControllers.erase(cid);
		}
	}

	void ControllerManager::Update(ControllerID jid, const Vector<U8>& states, const Vector<F32>& axes)
	{
		InputController& controller = sControllers.at(jid);

		controller.PreviousStates.clear();
		controller.PreviousStates.insert(controller.PreviousStates.begin(), controller.CurrentStates.begin(), controller.CurrentStates.end());

		controller.CurrentStates.clear();
		controller.CurrentStates.insert(controller.CurrentStates.begin(), states.begin(), states.end());

		controller.Axes.clear();
		controller.Axes.insert(controller.Axes.begin(), axes.begin(), axes.end());
	}

	BIT ControllerManager::IsButtonPressed(ControllerID cid, I32 button)
	{
		if (!sControllers.contains(cid)) return ESX_FALSE;
		return sControllers.at(cid).CurrentStates[button] == GLFW_PRESS && sControllers.at(cid).PreviousStates[button] == GLFW_PRESS;
	}

	BIT ControllerManager::IsButtonDown(ControllerID cid, I32 button)
	{
		if (!sControllers.contains(cid)) return ESX_FALSE;
		return sControllers.at(cid).CurrentStates[button] == GLFW_PRESS && sControllers.at(cid).PreviousStates[button] == GLFW_RELEASE;
	}

	BIT ControllerManager::IsButtonUp(ControllerID cid, I32 button)
	{
		if (!sControllers.contains(cid)) return ESX_FALSE;
		return sControllers.at(cid).CurrentStates[button] == GLFW_RELEASE && sControllers.at(cid).PreviousStates[button] == GLFW_PRESS;
	}

}