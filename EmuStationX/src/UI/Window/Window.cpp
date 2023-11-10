#include "UI/Window/Window.h"

namespace esx {
	Window::Window(const std::string& title, int32_t width, int32_t height)
		:	mWindowHandle(NULL)
	{
		if (!glfwInit()) {
			return;
		}

		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
		
		GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
		const GLFWvidmode* vidMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		glfwSetWindowPos(window, vidMode->width / 2 - width / 2, vidMode->height / 2 - height / 2);
		glfwShowWindow(window);
		glfwMakeContextCurrent(window);
		gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		glfwSwapInterval(1);

		glfwSetKeyCallback(window, [](GLFWwindow* windowRef, int key, int scancode, int action, int mods) {
			switch (action) {
				case GLFW_PRESS:
					InputManager::SetKey(key, true);
					break;

				case GLFW_RELEASE:
					InputManager::SetKey(key, false);
					break;
			}
		});

		mWindowHandle = window;
		mMaximized = false;
		mClosed = false;
	}

	Window::~Window()
	{
		glfwDestroyWindow(mWindowHandle);
		glfwTerminate();
	}

	void Window::show()
	{
		glfwShowWindow(mWindowHandle);
	}

	void Window::update()
	{
		glfwSwapBuffers(mWindowHandle);
		glfwPollEvents();
	}

	bool Window::isClosed()
	{
		return glfwWindowShouldClose(mWindowHandle);
	}

	void Window::Iconify() {
		glfwIconifyWindow(mWindowHandle);
	}

	void Window::Maximize(){
		glfwMaximizeWindow(mWindowHandle);
		mMaximized = true;
	}

	void Window::Restore() {
		glfwRestoreWindow(mWindowHandle);
		mMaximized = false;
	}

	void Window::Close() {
		mClosed = true;
	}

	void Window::GetPosition(int& x, int& y)
	{
		glfwGetWindowPos(mWindowHandle, &x, &y);
	}

	void Window::SetPosition(int x, int y)
	{
		glfwSetWindowPos(mWindowHandle, x, y);
	}

	void Window::GetSize(int& w, int& h)
	{
		glfwGetWindowSize(mWindowHandle, &w, &h);
	}

	void Window::SetSize(int w, int h)
	{
		glfwSetWindowSize(mWindowHandle, w, h);
	}
}