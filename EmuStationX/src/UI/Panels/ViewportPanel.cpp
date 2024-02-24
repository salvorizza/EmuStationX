#include "UI/Panels/ViewportPanel.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

#include <glad/glad.h>

namespace esx {



	ViewportPanel::ViewportPanel()
		: Panel("Viewport", false, true),
			mResizeWidth(160),
			mResizeHeight(144),
			mNeedResize(true)
	{}

	ViewportPanel::~ViewportPanel()
	{}

	void ViewportPanel::startFrame()
	{
		if (mFBO) {
			if (mNeedResize) {
				mFBO->resize(mResizeWidth, mResizeHeight);
				mNeedResize = false;
			}
			mFBO->bind();

			glViewport(0, 0, mFBO->width(), mFBO->height());

		}
	}

	void ViewportPanel::endFrame()
	{
		if (mFBO) {
			mFBO->unbind();
		}
	}

	void ViewportPanel::onImGuiRender()
	{
		ImVec2 size = ImGui::GetContentRegionAvail();
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		ImVec2 vMin = ImGui::GetWindowPos() + ImGui::GetWindowContentRegionMin();
		ImVec2 cursorPos = ImGui::GetCursorPos();

		ImVec2 screenSize = ImVec2(1024.0f, 512.0f);
		ImVec2 scales = size / screenSize;
		float scale = floor(std::min(scales.x, scales.y));
		ImVec2 newSize = screenSize * scale;

		ImVec2 offset = (size - newSize) / 2;

		uint32_t newWidth = (uint32_t)floor(newSize.x);
		uint32_t newHeight = (uint32_t)floor(newSize.y);
		if (!mFBO) {
			mFBO = std::make_shared<FrameBuffer>(newWidth, newHeight);
		} else {
			uint32_t fboWidth, fboHeight;

			mFBO->getSize(fboWidth, fboHeight);
			if (newWidth != fboWidth || newHeight != fboHeight) {
				mResizeWidth = newWidth;
				mResizeHeight = newHeight;
				mNeedResize = true;
			}
		}
		
		uint64_t textureID = mFBO->getColorAttachment();
		cursorPos += offset;

		ImGui::SetCursorPos(cursorPos);
		ImGui::Image(reinterpret_cast<void*>(textureID), ImVec2{ newSize.x, newSize.y }, ImVec2{ 0, 1 }, ImVec2{ 1, 0 });
	}

}