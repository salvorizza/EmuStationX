#include "UI/Graphics/VertexArray.h"

#include <glad/glad.h>

namespace esx {

	uint32_t getShaderDataTypeOpenGLPrimitiveType(ShaderType type) {
		switch (type) {
			case ShaderType::Float: return GL_FLOAT;
			case ShaderType::Float2: return GL_FLOAT;
			case ShaderType::Float4: return GL_FLOAT;
			case ShaderType::Uint4: return GL_UNSIGNED_BYTE;
		}
		return 0;
	}

	uint32_t getShaderDataTypeNumComponents(ShaderType type) {
		switch (type) {
			case ShaderType::Float: return 1;
			case ShaderType::Float2: return 2;
			case ShaderType::Float4: return 4;
			case ShaderType::Uint4: return 4;
		}
		return 0;
	}

	VertexArray::VertexArray()
		:	mRendererID(0)
	{
		glGenVertexArrays(1, &mRendererID);
		glBindVertexArray(mRendererID);
	}

	VertexArray::~VertexArray()
	{
		glDeleteBuffers(1, &mRendererID);
	}

	void VertexArray::bind()
	{
		glBindVertexArray(mRendererID);
	}

	void VertexArray::addVertexBuffer(const std::shared_ptr<VertexBuffer>& vbo)
	{
		const BufferLayout& layout = vbo->getLayout();
		size_t attribIndex = mVertexBuffers.size();

		for (const BufferElement& bufferElement : layout) {
			glEnableVertexAttribArray((GLuint)attribIndex);
			glVertexAttribPointer((GLuint)attribIndex, getShaderDataTypeNumComponents(bufferElement.Type), getShaderDataTypeOpenGLPrimitiveType(bufferElement.Type), bufferElement.Normalized, layout.getStride(), (const void*)bufferElement.Offset);
			attribIndex++;
		}
	}

	void VertexArray::unbind()
	{
		glBindVertexArray(0);
	}

}