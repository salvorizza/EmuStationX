#pragma once

#include <cstdint>
#include "Texture2D.h"

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

		SharedPtr<Texture2D> getColorAttachment() const { return mColorAttachment; }
		
	private:
		void invalidate();
	private:
		uint32_t mRendererID;
		SharedPtr<Texture2D> mColorAttachment;
		//uint32_t mColorAttachment;
		int32_t mWidth, mHeight;
	};

}
