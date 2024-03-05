#pragma once

#include <memory>

#include "Panel.h"
#include "UI/Graphics/FrameBuffer.h"

namespace esx {

	class ViewportPanel : public Panel {
	public:
		ViewportPanel();
		~ViewportPanel();

		void setFrame(const SharedPtr<FrameBuffer>& frame) { mFrame = frame; }

	protected:
		virtual void onImGuiRender() override;

	private:
		SharedPtr<FrameBuffer> mFrame;
	};

}