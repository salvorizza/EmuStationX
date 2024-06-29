#include "UI/Panels/TTYPanel.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

namespace esx {

	TTYPanel::TTYPanel()
		: Panel("TTYPanel", false, true)
	{}

	TTYPanel::~TTYPanel()
	{}


	void TTYPanel::onImGuiRender()
	{
		if (ImGui::BeginChild("Log")) {
			ImGui::TextUnformatted(mInstance->mTTY.str().data());
			//ImGui::SetScrollHereY(1.0f);
			ImGui::EndChild();
		}
	}
}