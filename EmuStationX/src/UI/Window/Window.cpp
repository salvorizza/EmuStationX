#include "UI/Window/Window.h"

namespace esx {

	static void APIENTRY debugCallbackOpenGL(GLenum source​, GLenum type​, GLuint id​, GLenum severity​, GLsizei length​, const GLchar* message​, const void* userParam​) {
		switch (type​) {
			case GL_DEBUG_TYPE_ERROR: {
				ESX_CORE_LOG_ERROR("{}​", message​);
				break;
			}
			default: {
				//ESX_CORE_LOG_INFO("{}​", message​);
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
				image.pixels = stbi_load_from_memory(result[i].Data.data(), result[i].Data.size(), &image.width, &image.height, &channels, 4);
			}

			glfwSetWindowIcon(window, images.size(), images.data());
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