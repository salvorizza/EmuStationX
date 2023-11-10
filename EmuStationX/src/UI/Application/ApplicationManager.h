#pragma once

#include <memory>

#include "Application.h"
#include "UI/Window/ImGuiManager.h"
#include "UI/Window/Window.h"

namespace esx {

	class ApplicationManager {
	public:
		ApplicationManager();
		~ApplicationManager();

		void run(const std::shared_ptr<Application>& pApplication);
	private:
		std::shared_ptr<Application> mApplication;
		std::shared_ptr<ImGuiManager> mImGuiManager;
		std::shared_ptr<Window> mWindow;
	};

}
