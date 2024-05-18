#pragma once

#include <string>

#include "UI/Window/ImGuiManager.h"

namespace esx {

	class Application {
	public:
		Application(const std::string& name, const std::string& icoPath = "") : mName(name), mICOPath(icoPath) {}
		virtual ~Application() = default;

		virtual void onSetup() = 0;
		virtual void onUpdate() = 0;
		virtual void onRender() = 0;
		virtual void onCleanUp() = 0;
		virtual void onImGuiRender(const std::shared_ptr<ImGuiManager>& pManager, const std::shared_ptr<Window>& pWindow) = 0;

		const std::string& getName() const { return mName; }
		const std::string& getICOPath() const { return mICOPath; }

	private:
		std::string mName;
		std::string mICOPath;
	};

}