#include "ControllerManager.h"

namespace esx {

	UnorderedMap<ControllerID, InputController> ControllerManager::sControllers = {};

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