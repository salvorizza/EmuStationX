#pragma once

#include <string>

#include "UI/Window/ImGuiManager.h"

namespace esx {

	class Application {
	public:
		Application(const std::string& name) : mName(name) {}
		virtual ~Application() = default;

		virtual void onSetup() = 0;
		virtual void onUpdate() = 0;
		virtual void onRender() = 0;
		virtual void onImGuiRender(const std::shared_ptr<ImGuiManager>& pManager, const std::shared_ptr<Window>& pWindow) = 0;

		const std::string& getName() const { return mName; }

	private:
		std::string mName;
	};

}