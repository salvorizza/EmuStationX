#pragma once

#include <cstdint>

namespace esx {

	class FrameBuffer {
	public:
		FrameBuffer(int32_t width, int32_t height);
		~FrameBuffer();

		void bind();
		void unbind();
		void resize(int32_t width, int32_t height);

		void getSize(uint32_t& width, uint32_t& height) const {
			width = mWidth;
			height = mHeight;
		}

		uint32_t width() const { return mWidth; }
		uint32_t height() const { return mHeight; }

		uint32_t getColorAttachment() const { return mColorAttachment; }
		
	private:
		void invalidate();
	private:
		uint32_t mRendererID;
		uint32_t mColorAttachment, mColorAttachmentMS;
		int32_t mWidth, mHeight;
	};

}
