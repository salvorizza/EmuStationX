#include "UI/Graphics/FrameBuffer.h"

#include <glad/glad.h>

namespace esx {



	FrameBuffer::FrameBuffer(int32_t width, int32_t height)
		:	mRendererID(0),
			mColorAttachment(0),
			mWidth(width),
			mHeight(height)
	{
	}

	FrameBuffer::~FrameBuffer()
	{
		glDeleteFramebuffers(1, &mRendererID);
	}

	void FrameBuffer::init()
	{
		invalidate();
	}

	void FrameBuffer::bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, mRendererID);
	}

	void FrameBuffer::unbind()
	{
		glBindFramebuffer(GL_READ_FRAMEBUFFER, mRendererID);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(0, 0, mWidth, mHeight, 0, 0, mWidth, mHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void FrameBuffer::resize(int32_t width, int32_t height)
	{
		mWidth = width;
		mHeight = height;

		invalidate();
	}

	void FrameBuffer::invalidate()
	{
		if (mRendererID != 0) {
			mColorAttachment.reset();
			mRendererID =  0;
		}

		glGenFramebuffers(1, &mRendererID);
		glBindFramebuffer(GL_FRAMEBUFFER, mRendererID);
		mColorAttachment->bind();
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mColorAttachment->getRendererID(), 0);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			int i = 0;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}