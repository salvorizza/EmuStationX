#include "UI/Window/Window.h"

#include <glad/glad.h>

#include "ControllerManager.h"

namespace esx {

	static void APIENTRY debugCallbackOpenGL(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
		switch (type) {
			case GL_DEBUG_TYPE_ERROR: {
				ESX_CORE_LOG_ERROR("{}", message);
				break;
			}
			default: {
				//ESX_CORE_LOG_INFO("{}", message);
				break;
			}
		}
	}

	Window::Window(const std::string& title, int32_t width, int32_t height, const std::string& icoPath)
		:	mWindowHandle(NULL)
	{
		if (!glfwInit()) {
			return;
		}

		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
		glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
		
		GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
		const GLFWvidmode* vidMode = glfwGetVideoMode(glfwGetPrimaryMonitor());
		glfwSetWindowPos(window, vidMode->width / 2 - width / 2, vidMode->height / 2 - height / 2);

		if (!icoPath.empty()) {
			auto result = ReadICO(icoPath);

			Vector<GLFWimage> images(result.size());
			for (int i = 0; i < result.size(); i++) {
				GLFWimage& image = images[i];

				int channels;
				image.pixels = stbi_load_from_memory(result[i].Data.data(), (int)result[i].Data.size(), &image.width, &image.height, &channels, 4);
			}

			glfwSetWindowIcon(window, (int)images.size(), images.data());
		}

		glfwShowWindow(window);
		glfwMakeContextCurrent(window);
		gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		glDebugMessageCallback(debugCallbackOpenGL, nullptr);
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

		for (int jid = 0; jid < GLFW_JOYSTICK_LAST; jid++) {
			if (glfwJoystickPresent(jid) == GLFW_TRUE) {
				ControllerManager::Connect(ControllerID(jid), glfwGetJoystickName(jid), (glfwJoystickIsGamepad(jid) == GLFW_TRUE) ? ESX_TRUE : ESX_FALSE);
			}
		}

		glfwSetJoystickCallback([](int jid, int event) {
			switch (event) {
				case GLFW_CONNECTED: {
					ControllerManager::Connect(ControllerID(jid), glfwGetJoystickName(jid), (glfwJoystickIsGamepad(jid) == GLFW_TRUE) ? ESX_TRUE : ESX_FALSE);
					break;
				}
				case GLFW_DISCONNECTED: {
					ControllerManager::Disconnect(ControllerID(jid));
					break;
				}
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
		InputManager::Update();

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
				} else {
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