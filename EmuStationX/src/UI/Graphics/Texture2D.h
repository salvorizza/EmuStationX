#pragma once

#include "Base/Base.h"
#include "PixelBuffer.h"

namespace esx {

	enum InternalFormat {
		R16,
		R8,
		RGB8
	};

	enum DataFormat {
		RED,
		RGB
	};

	enum DataType {
		UnsignedByte,
		UnsignedShort
	};

	class Texture2D {
	public:
		Texture2D(U32 slot = 0);
		~Texture2D();

		void bind();
		void unbind();
		void setData(void* data, U32 width, U32 height, InternalFormat internalFormat, DataType type, DataFormat format);
		void copy(const SharedPtr<PixelBuffer>& pixelBuffer);

		U32 getRendererID() const { return mRendererID; }


	private:
		U32 mRendererID = 0;
		U32 mSlot = 0;
		U32 mWidth = 0;
		U32 mHeight = 0;
		InternalFormat mInternalFormat;
		DataType mDataType;
		DataFormat mDataFormat;
	};

}