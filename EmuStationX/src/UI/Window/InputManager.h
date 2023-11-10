#pragma once


#include <GLFW/glfw3.h>

#include <memory>


namespace esx {

	class InputManager {
	public:
		InputManager() = delete;
		~InputManager() = default;

		static void Init();
		static void Update();
		static void SetKey(size_t key, bool value);

		static bool IsKeyPressed(size_t key);
		static bool IsKeyDown(size_t key);
		static bool IsKeyUp(size_t key);
	private:
		static bool sKeys[GLFW_KEY_LAST];
		static bool sPrevKeys[GLFW_KEY_LAST];
	};

}