#pragma once

#include <Core/R3000.h>
#include <imgui_console/imgui_console.h>

#include "Panel.h"

namespace esx {

	class ConsolePanel : public Panel {
	public:
		ConsolePanel();
		~ConsolePanel();

		ImGuiConsole& getInternalConsole() { return mConsole; }

	protected:
		virtual void onImGuiRender() override;

	private:
		ImGuiConsole mConsole;
	};

}