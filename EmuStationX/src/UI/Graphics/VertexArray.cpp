#include "UI/Graphics/VertexArray.h"

#include <glad/glad.h>

namespace esx {

	GLenum getShaderDataTypeOpenGLPrimitiveType(ShaderType type) {
		switch (type) {
			case ShaderType::Float: return GL_FLOAT;
			case ShaderType::Float2: return GL_FLOAT;
			case ShaderType::Float4: return GL_FLOAT;
			case ShaderType::UByte2: return GL_UNSIGNED_BYTE;
			case ShaderType::UByte3: return GL_UNSIGNED_BYTE;
			case ShaderType::UByte4: return GL_UNSIGNED_BYTE;
			case ShaderType::Short2: return GL_SHORT;
		}
		return 0;
	}

	GLint getShaderDataTypeNumComponents(ShaderType type) {
		switch (type) {
			case ShaderType::Float: return 1;
			case ShaderType::Float2: return 2;
			case ShaderType::Float4: return 4;

			case ShaderType::Short2: return 2;

			case ShaderType::UByte2: return 2;
			case ShaderType::UByte3: return 3;
			case ShaderType::UByte4: return 4;
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
		vbo->bind();
		const BufferLayout& layout = vbo->getLayout();
		GLuint attribIndex = mVertexBuffers.size();

		for (const BufferElement& bufferElement : layout) {
			glEnableVertexAttribArray(attribIndex);
			GLint numComponents = getShaderDataTypeNumComponents(bufferElement.Type);
			GLenum primitiveType = getShaderDataTypeOpenGLPrimitiveType(bufferElement.Type);
			glVertexAttribPointer(attribIndex, numComponents, primitiveType, bufferElement.Normalized, layout.getStride(), (const void*)bufferElement.Offset);
			attribIndex++;
		}
	}

	void VertexArray::unbind()
	{
		glBindVertexArray(0);
	}

}