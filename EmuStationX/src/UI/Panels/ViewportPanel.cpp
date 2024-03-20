#include "UI/Panels/ViewportPanel.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

#include <glad/glad.h>

namespace esx {



	ViewportPanel::ViewportPanel()
		: Panel("Viewport", false, true)
	{}

	ViewportPanel::~ViewportPanel()
	{}


	void ViewportPanel::onImGuiRender()
	{
		ImVec2 size = ImGui::GetContentRegionAvail();
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 cursorPos = ImGui::GetCursorPos();

		ImVec2 frameSize = ImVec2((float)mFrame->width(), (float)mFrame->height());
		ImVec2 imageSize = ImVec2();
		ImVec2 offset = ImVec2();
		if (size.y < size.x) {
			float aspectRatio = frameSize.x / frameSize.y;

			imageSize.y = size.y;
			imageSize.x = size.y * aspectRatio;
			offset = ImVec2((size.x - imageSize.x) / 2, 0);
		} else {
			float aspectRatio = frameSize.y / frameSize.x;

			imageSize.x = size.x;
			imageSize.y = size.x * aspectRatio;
			offset = ImVec2(0, (size.y - imageSize.y) / 2);
		}
		
		uint64_t textureID = mFrame->getColorAttachment()->getRendererID();
		ImGui::SetCursorPos(cursorPos + offset);
		ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ imageSize.x, imageSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
	}

}