#include "UI/Graphics/IndexBuffer.h"

#include <glad/glad.h>

namespace esx {

	uint32_t dataUsageToOpenGLDataUsage2(VertexBufferDataUsage dataUsage) {
		switch (dataUsage) {
			case VertexBufferDataUsage::Dynamic: return GL_DYNAMIC_DRAW;
			case VertexBufferDataUsage::Static: return GL_STATIC_DRAW;
		}
		return 0;
	}

	IndexBuffer::IndexBuffer()
	{
		glGenBuffers(1, &mRendererID);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mRendererID);
	}

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

	void IndexBuffer::setData(void* data, size_t size, VertexBufferDataUsage dataUsage)
	{
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, data, dataUsageToOpenGLDataUsage2(dataUsage));
	}

	void IndexBuffer::copyData(void* data, size_t size)
	{
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, size, data);
	}
}