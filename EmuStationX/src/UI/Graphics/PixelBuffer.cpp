#include "PixelBuffer.h"

#include <glad/glad.h>

#include "Utils/LoggingSystem.h"

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
		ESX_CORE_LOG_TRACE(__FUNCTION__);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, mRendererID);
	}

	void PixelBuffer::unbind()
	{
		ESX_CORE_LOG_TRACE(__FUNCTION__);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	}

	void PixelBuffer::setData(void* data, size_t size, U32 flags)
	{
		ESX_CORE_LOG_TRACE(__FUNCTION__);
		glBufferStorage(GL_PIXEL_UNPACK_BUFFER, size, data, fromFlagsToBitField(flags));
		mSize = size;
		mFlags = flags;
	}

	void* PixelBuffer::mapBuffer()
	{
		ESX_CORE_LOG_TRACE(__FUNCTION__);
		return glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
	}

	void* PixelBuffer::mapBufferRange()
	{
		ESX_CORE_LOG_TRACE(__FUNCTION__);
		return glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, mSize, fromFlagsToBitField(mFlags));
	}

	void PixelBuffer::unmapBuffer()
	{
		ESX_CORE_LOG_TRACE(__FUNCTION__);
		glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
	}

}