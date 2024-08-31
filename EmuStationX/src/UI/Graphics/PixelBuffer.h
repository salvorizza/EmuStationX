#pragma once

#include "Base/Base.h"

namespace esx {

	enum BufferMode {
		Write = 1 << 0,
		Persistent = 1 << 1,
		Read = 1 << 2
	};

	class PixelBuffer {
	public:
		PixelBuffer();
		~PixelBuffer();

		void bind();
		void unbind();
		void setData(void* data, size_t size, U32 flags);
		void* mapBuffer();
		void* mapBufferRange();
		void unmapBuffer();

	private:
		U32 mRendererID = 0;
		size_t mSize = 0;
		U32 mFlags = 0;
	};

}