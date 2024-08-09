#pragma once

#include "Base/Base.h"
#include "PixelBuffer.h"

namespace esx {

	enum InternalFormat {
		R16,
		R8,
		RGB8,
		RGBA8,
		RGB5_A1
	};

	enum DataFormat {
		RED,
		RGB,
		RGBA
	};

	enum DataType {
		UnsignedByte,
		UnsignedShort,
		UnsignedShort555_1,
		UnsignedShort1_555
	};

	class Texture2D {
	public:
		Texture2D(U32 slot = 0);
		Texture2D(U32 renderedID, U32 slot);
		~Texture2D();

		void bind();
		void unbind();
		void setData(void* data, U32 width, U32 height, InternalFormat internalFormat, DataType type, DataFormat format);
		void copy(const SharedPtr<PixelBuffer>& pixelBuffer);
		void getPixels(void** pixels);
		void updatePixels(void* pixels);
		void setPixel(U32 x,U32 y, const void* pixelData);
		void setPixels(U32 x, U32 y, U32 width, U32 height, const void* pixelData);
		void getPixel(U32 x, U32 y, void** pixelData);
		SharedPtr<Texture2D> createView(InternalFormat viewFormat);

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