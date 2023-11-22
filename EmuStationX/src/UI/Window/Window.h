#pragma once

#include <cstdint>
#include <string>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "InputManager.h"

#include "UI/Utils.h"
#include "stb_image.h"


namespace esx {

	class Window {
	public:
		Window(const std::string& title,int32_t width,int32_t height,const std::string& icoPath);
		~Window();
		
		void show();
		void update();
		bool isClosed();

		void Iconify();
		void Restore();
		void Maximize();
		void Close();

		void GetPosition(int& x, int& y);
		void SetPosition(int x, int y);

		void GetSize(int& w, int& h);
		void SetSize(int w, int h);

		inline bool isMaximized() const { return mMaximized; }

		GLFWwindow* getHandle() { return mWindowHandle; }
	private:
		GLFWwindow* mWindowHandle;
		bool mMaximized;
		bool mClosed;
	};

}
