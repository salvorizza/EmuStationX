#include "PixelBuffer.h"

#include <glad/glad.h>

namespace esx {

	static GLbitfield fromFlagsToBitField(U32 flags) {
		GLbitfield result = 0;

		if (flags & BufferMode::Write) {
			result |= GL_MAP_WRITE_BIT;
		}

		if (flags & BufferMode::Persistent) {
			result |= GL_MAP_PERSISTENT_BIT;
		}

		if (flags & BufferMode::Read) {
			result |= GL_MAP_READ_BIT;
		}

		return result;
	}

	PixelBuffer::PixelBuffer()
	{
		glGenBuffers(1, &mRendererID);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, mRendererID);
	}

	PixelBuffer::~PixelBuffer()
	{
		glDeleteBuffers(1, &mRendererID);
	}

	void PixelBuffer::bind()
	{
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, mRendererID);
	}

	void PixelBuffer::unbind()
	{
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	}

	void PixelBuffer::setData(void* data, size_t size, U32 flags)
	{
		glBufferStorage(GL_PIXEL_UNPACK_BUFFER, size, data, fromFlagsToBitField(flags));
		mSize = size;
		mFlags = flags;
	}

	void* PixelBuffer::mapBufferRange()
	{
		return glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, mSize, fromFlagsToBitField(mFlags));
	}

}