#include "UI/Window/ImGuiManager.h"
#include "UI/Window/FontAwesome5.h"


#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include <backends/imgui_impl_glfw.cpp>
#include <backends/imgui_impl_opengl3.cpp>

#include "stb_image.h"

namespace esx {

	bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height)
	{
		// Load from file
		int image_width = 0;
		int image_height = 0;
		unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
		if (image_data == NULL)
			return false;

		// Create a OpenGL texture identifier
		GLuint image_texture;
		glGenTextures(1, &image_texture);
		glBindTexture(GL_TEXTURE_2D, image_texture);

		// Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
		stbi_image_free(image_data);


		glGenerateMipmap(GL_TEXTURE_2D);

		// Setup filtering parameters for display
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same



		*out_texture = image_texture;
		*out_width = image_width;
		*out_height = image_height;

		return true;
	}

	bool LoadTextureFromMemory(const uint8_t* data, size_t size, GLuint* out_texture, int* out_width, int* out_height) {
		// Load from file
		int image_width = 0;
		int image_height = 0;
		int n = 0;
		unsigned char* image_data = (unsigned char*)stbi_load_from_memory(data, size, &image_width, &image_height, &n, 4);
		if (image_data == NULL)
			return false;

		// Create a OpenGL texture identifier
		GLuint image_texture;
		glGenTextures(1, &image_texture);
		glBindTexture(GL_TEXTURE_2D, image_texture);

		// Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
		stbi_image_free(image_data);


		glGenerateMipmap(GL_TEXTURE_2D);

		// Setup filtering parameters for display
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same



		*out_texture = image_texture;
		*out_width = image_width;
		*out_height = image_height;

		return true;
	}

	ImGuiManager::ImGuiManager(const std::shared_ptr<Window>& pWindow)
		: mWindow(pWindow)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

		ImGui_ImplGlfw_InitForOpenGL(pWindow->getHandle(), true);
		ImGui_ImplOpenGL3_Init("#version 450");

		setDarkThemeColors();

		static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		ImFontConfig icons_config; icons_config.MergeMode = true; icons_config.PixelSnapH = true;

		float fontSize = 16.0f;
		io.FontDefault = io.Fonts->AddFontFromFileTTF("commons/fonts/opensans/OpenSans-Regular.ttf", fontSize);
		io.Fonts->AddFontFromFileTTF("commons/fonts/fontawesome/" FONT_ICON_FILE_NAME_FAR, 15.0f, &icons_config, icons_ranges);
	}

	ImGuiManager::~ImGuiManager()
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}


	void ImGuiManager::startFrame()
	{
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	void ImGuiManager::endFrame()
	{
		ImGuiIO& io = ImGui::GetIO();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}
	void ImGuiManager::setDarkThemeColors()
	{
		auto& colors = ImGui::GetStyle().Colors;


		//colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };

		colors[ImGuiCol_MenuBarBg] = ImVec4{ 0.09f, 0.09f, 0.09f, 1.0f };
		colors[ImGuiCol_WindowBg] = ImVec4{ 0.09f, 0.09f, 0.09f, 1.0f };
		colors[ImGuiCol_PopupBg] = ImVec4{ 0.09f, 0.09f, 0.09f, 1.0f };

		// Headers
		colors[ImGuiCol_Header] = ImVec4{ 0.18f, 0.18f, 0.18f, 1.0f };
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.21f, 0.21f, 0.21f, 1.0f };
		colors[ImGuiCol_HeaderActive] = ImVec4{ 0.21f, 0.21f, 0.21f, 1.0f };

		// Buttons
		colors[ImGuiCol_Button] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ 0.05f, 0.0505f, 0.051f, 1.0f };

		// Frame BG
		colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Tabs
		colors[ImGuiCol_Tab] = ImVec4{ 0.09f, 0.09f, 0.09f, 1.0f };
		colors[ImGuiCol_TabHovered] = ImVec4{ 0.0f, 0.43f, 0.87f, 0.7f };
		colors[ImGuiCol_TabActive] = ImVec4{ 0.0f, 0.43f, 0.87f, 1.0f };
		colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.09f, 0.09f, 0.09f, 1.0f };
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.0f, 0.43f, 0.87f, 1.0f };
		colors[ImGuiCol_Separator] = ImVec4{ 0.09f,.09f,.09f, 1.0f };
		colors[ImGuiCol_SeparatorActive] = ImVec4{ 0.21f,0.21f,0.21f, 1.0f };
		colors[ImGuiCol_SeparatorHovered] = ImVec4{ 0.21f,0.21f,0.21f, 1.0f };

		// Title
		colors[ImGuiCol_TitleBg] = ImVec4{ 0.09f, 0.09f, 0.09f, 1.0f };
		colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.09f, 0.09f, 0.09f, 1.0f };
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.09f, 0.09f, 0.09f, 1.0f };

		//Tables
		colors[ImGuiCol_TableRowBg] = ImVec4{ 0.09f, 0.09f, 0.09f, 1.0f };
		colors[ImGuiCol_TableRowBgAlt] = ImVec4{ 0.18f, 0.18f, 0.18f, 1.0f };
		colors[ImGuiCol_TableBorderLight] = ImVec4{ 0.0f, 0.0f, 0.0f, 1.0f };
		colors[ImGuiCol_TableBorderStrong] = ImVec4{ 0.0f, 0.0f, 0.0f, 1.0f };

		//Tables
		colors[ImGuiCol_Button] = ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ 0.21f, 0.21f, 0.21f, 1.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.21f, 0.21f, 0.21f, 1.0f };

		ImGui::GetStyle().WindowRounding = 3.0f;
		ImGui::GetStyle().FrameRounding = 5.0f;
	}

	IconData ImGuiManager::LoadIconResource(const char* imagePath)
	{
		if (mIcons.find(imagePath) == mIcons.end()) {
			IconData data;
			bool ret = LoadTextureFromFile(imagePath, &data.textureID, &data.Width, &data.Height);
			IM_ASSERT(ret);
			if (ret) {
				mIcons[imagePath] = data;
			}
		}
		return mIcons[imagePath];
	}

	IconData ImGuiManager::LoadIconResource(const char* name, const uint8_t* data, size_t size)
	{
		if (mIcons.find(name) == mIcons.end()) {
			IconData iconData;
			bool ret = LoadTextureFromMemory(data, size, & iconData.textureID, &iconData.Width, &iconData.Height);
			IM_ASSERT(ret);
			if (ret) {
				mIcons[name] = iconData;
			}
		}
		return mIcons[name];
	}
	IconData ImGuiManager::GetIconResource(const char* name)
	{
		IconData iconData;

		if (mIcons.find(name) == mIcons.end()) {
			return iconData;
		}

		return mIcons.at(name);
	}

	bool ImGuiManager::ExistsIconResource(const char* name) {
		return mIcons.find(name) != mIcons.end();
	}

	bool ImGuiManager::ReleaseIconResource(const char* name) {
		auto it = mIcons.find(name);

		if (it == mIcons.end()) {
			return false;
		}

		glDeleteTextures(1, &it->second.textureID);
		mIcons.erase(it);

		return true;
	}

}


