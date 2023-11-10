#include "UI/Graphics/IndexBuffer.h"

#include <glad/glad.h>

namespace esx {
	IndexBuffer::IndexBuffer(uint32_t* indices, uint32_t num)
		:	mRendererID(0),
			mNumIndices(0)
	{
		glGenBuffers(1, &mRendererID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mRendererID);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, num * sizeof(uint32_t), indices, GL_STATIC_DRAW);
		mNumIndices = num;
	}

	IndexBuffer::~IndexBuffer()
	{
		glDeleteBuffers(1, &mRendererID);
	}

	void IndexBuffer::bind()
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mRendererID);
	}

	void IndexBuffer::unbind()
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
}