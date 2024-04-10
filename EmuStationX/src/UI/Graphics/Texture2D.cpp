#include "Texture2D.h"

#include <glad/glad.h>

namespace esx {

	static GLenum fromInternalFormat(InternalFormat internalFormat) {
		switch (internalFormat) {
			case InternalFormat::R16: return GL_R16UI;
			case InternalFormat::R8: return GL_R8UI;
			case InternalFormat::RGB8: return GL_RGB8;
			case InternalFormat::RGBA8: return GL_RGBA8;
			case InternalFormat::RGB5_A1: return GL_RGB5_A1;
		}
		return 0;
	}

	static GLenum fromDataFormat(DataFormat dataFormat) {
		switch (dataFormat) {
			case DataFormat::RED: return GL_RED_INTEGER;
			case DataFormat::RGB: return GL_RGB;
			case DataFormat::RGBA: return GL_RGBA;
		}
		return 0;
	}


	static GLenum fromDataType(DataType dataType) {
		switch (dataType) {
			case DataType::UnsignedByte: return GL_UNSIGNED_BYTE;
			case DataType::UnsignedShort: return GL_UNSIGNED_SHORT;
			case DataType::UnsignedShort555_1: return GL_UNSIGNED_SHORT_5_5_5_1;
		}
		return 0;
	}

	Texture2D::Texture2D(U32 slot)
		: mSlot(slot)
	{
		glGenTextures(1, &mRendererID);
		glActiveTexture(GL_TEXTURE0 + slot);
		glBindTexture(GL_TEXTURE_2D, mRendererID);

		/* Set the texture wrapping parameters. */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		/* Set texture filtering parameters. */
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	Texture2D::~Texture2D()
	{
		glDeleteTextures(1, &mRendererID);
	}

	void Texture2D::bind()
	{
		glActiveTexture(GL_TEXTURE0 + mSlot);
		glBindTexture(GL_TEXTURE_2D, mRendererID);
	}

	void Texture2D::unbind()
	{
		glActiveTexture(GL_TEXTURE0 + mSlot);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void Texture2D::setData(void* data, U32 width, U32 height, InternalFormat internalFormat, DataType type, DataFormat format)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, fromInternalFormat(internalFormat), width, height, 0, fromDataFormat(format), fromDataType(type), data);
		mWidth = width;
		mHeight = height;
		mDataFormat = format;
		mDataType = type;
	}

	void Texture2D::copy(const SharedPtr<PixelBuffer>& pixelBuffer)
	{
		pixelBuffer->bind();
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mWidth, mHeight, fromDataFormat(mDataFormat), fromDataType(mDataType), nullptr);
	}

	void Texture2D::updatePixels(void* pixels)
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, mWidth, mHeight, fromDataFormat(mDataFormat), fromDataType(mDataType), pixels);
	}

	void Texture2D::setPixel(U32 x, U32 y, const void* pixelData)
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, 1, 1, fromDataFormat(mDataFormat), fromDataType(mDataType), pixelData);
	}

	void Texture2D::getPixel(U32 x, U32 y, void** pixelData)
	{
		glGetTexImage(GL_TEXTURE_2D, 0, fromDataFormat(mDataFormat), fromDataType(mDataType), *pixelData);
	}

	void Texture2D::getPixels(void** pixels)
	{
		glGetTexImage(GL_TEXTURE_2D, 0, fromDataFormat(mDataFormat), fromDataType(mDataType), *pixels);
	}
}