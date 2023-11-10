#pragma once

#include <memory>

#include "Panel.h"
#include "UI/Graphics/FrameBuffer.h"

namespace esx {

	class ViewportPanel : public Panel {
	public:
		ViewportPanel();
		~ViewportPanel();

		void startFrame();
		void endFrame();

		uint32_t width() const { return mFBO ? mFBO->width() : 0; }
		uint32_t height() const { return mFBO ? mFBO->height() : 0; }

	protected:
		virtual void onImGuiRender() override;

	private:
		std::shared_ptr<FrameBuffer> mFBO;
		uint32_t mResizeWidth, mResizeHeight;
		bool mNeedResize;
	};

}