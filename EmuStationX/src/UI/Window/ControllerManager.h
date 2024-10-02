#pragma once

#include "Base/Base.h"

#include "Window.h"

namespace esx {

	using ControllerID = I32;

	struct InputController {
		ControllerID ID = { 0 };
		StringView Name = "";
		BIT IsGamepad = ESX_FALSE;
		Vector<U8> PreviousStates = {};
		Vector<U8> CurrentStates = {};
		Vector<F32> Axes = {};

		InputController() = default;

		InputController(ControllerID id, StringView name, BIT isGamepad)
			: ID(id), Name(name), IsGamepad(isGamepad)
		{}
	};

	class ControllerManager {
	public:
		ControllerManager() = delete;
		~ControllerManager() = default;

		static void Connect(ControllerID cid, StringView name, BIT isGamepad);
		static void Disconnect(ControllerID cid);
		static void Update(ControllerID jid, const Vector<U8>& states, const Vector<F32>& axes);

		static BIT IsButtonPressed(I32 button);
		static BIT IsButtonDown(I32 button);
		static BIT IsButtonUp(I32 button);
	private:
		static UnorderedMap<ControllerID, InputController> sControllers;
	};

}