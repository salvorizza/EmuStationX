#include "UI/Graphics/FrameBuffer.h"

#include <glad/glad.h>

namespace esx {



	FrameBuffer::FrameBuffer(int32_t width, int32_t height)
		:	mRendererID(0),
			mColorAttachment(0),
			mWidth(width),
			mHeight(height)
	{
		invalidate();
	}

	FrameBuffer::~FrameBuffer()
	{
		glDeleteTextures(1, &mColorAttachment);
		glDeleteFramebuffers(1, &mRendererID);
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
			glDeleteTextures(1, &mColorAttachment);
			glDeleteTextures(1, &mColorAttachmentMS);
			glDeleteFramebuffers(1, &mRendererID);
			mRendererID = mColorAttachment = mColorAttachmentMS =  0;
		}

		glGenFramebuffers(1, &mRendererID);
		glBindFramebuffer(GL_FRAMEBUFFER, mRendererID);

		glGenTextures(1, &mColorAttachment);
		glGenTextures(1, &mColorAttachmentMS);

		glGenTextures(1, &mColorAttachmentMS);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, mColorAttachmentMS);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, mWidth, mHeight, GL_TRUE);
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, mColorAttachmentMS, 0);

		glBindTexture(GL_TEXTURE_2D, mColorAttachment);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mWidth, mHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mColorAttachment, 0);

		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE) {
			int i = 0;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
}