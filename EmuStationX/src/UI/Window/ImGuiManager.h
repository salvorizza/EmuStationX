#pragma once

#include <memory>

#include "UI/Window/Window.h"

#include <imgui.h>

#include <unordered_map>

namespace esx {

	struct IconData {
		U32 textureID;
		I32 Width, Height;

		IconData()
			:	textureID(0),Width(0),Height(0)
		{}
	};

	class ImGuiManager {
	public:
		ImGuiManager(const std::shared_ptr<Window>& pWindow);
		~ImGuiManager();

		void startFrame();
		void endFrame();
		void cleanUp();

		void setDarkThemeColors();

		ImFont* getSmallIconFont() { return mSmallIconFont; }
		ImFont* getLargeIconFont() { return mLargeIconFont; }

		IconData LoadIconResource(const char* imagePath);
		IconData LoadIconResource(const char* name, const uint8_t* data, size_t size);
		IconData GetIconResource(const char* name);
		bool ReleaseIconResource(const char* name);

		bool ExistsIconResource(const char* name);

	private:
		std::shared_ptr<Window> mWindow;

		ImFont* mSmallIconFont;
		ImFont* mLargeIconFont;

		std::unordered_map<std::string, IconData> mIcons;
	};

}
