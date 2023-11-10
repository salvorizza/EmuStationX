#include "UI/Window/InputManager.h"

namespace esx {

	bool InputManager::sKeys[];
	bool InputManager::sPrevKeys[];

	void InputManager::Init()
	{
		std::memset(sKeys, false, sizeof(sKeys));
		std::memset(sPrevKeys, false, sizeof(sPrevKeys));
	}

	void InputManager::Update()
	{
		std::memcpy(sPrevKeys, sKeys, sizeof(sKeys));
	}

	void InputManager::SetKey(size_t key, bool value)
	{
		sKeys[key] = value;
	}

	bool InputManager::IsKeyPressed(size_t key)
	{
		return sKeys[key];
	}

	bool InputManager::IsKeyDown(size_t key)
	{
		return sKeys[key] && !sPrevKeys[key];
	}

	bool InputManager::IsKeyUp(size_t key)
	{
		return !sKeys[key] && sPrevKeys[key];
	}

}