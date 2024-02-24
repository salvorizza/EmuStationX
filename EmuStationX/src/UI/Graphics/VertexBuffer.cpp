#include "UI/Graphics/VertexBuffer.h"

#include "glad/glad.h"

namespace esx {

	uint32_t dataUsageToOpenGLDataUsage(VertexBufferDataUsage dataUsage) {
		switch (dataUsage) {
		case VertexBufferDataUsage::Dynamic: return GL_DYNAMIC_DRAW;
		case VertexBufferDataUsage::Static: return GL_STATIC_DRAW;
		}
		return 0;
	}

	uint32_t getShaderDataTypeSize(ShaderType type) {
		switch (type) {
			case ShaderType::Float: return  4 * 1;
			case ShaderType::Float2: return 4 * 2;
			case ShaderType::Float4: return 4 * 4;

			case ShaderType::Short2: return 2 * 2;

			case ShaderType::UByte2: return 1 * 2;
			case ShaderType::UByte3: return 1 * 3;
			case ShaderType::UByte4: return 1 * 4;
		}
		return 0;
	}


	BufferElement::BufferElement(const std::string& name, ShaderType type, bool normalized) 
		:	Name(name), Type(type), Offset(0), Normalized(normalized)
	{}

	BufferLayout::BufferLayout(std::initializer_list<BufferElement> list) 
		:	mBufferElements(list),
			mStride(0)
	{
		calculateOffsetsAndStride();
	}

	BufferLayout::~BufferLayout() {
		mBufferElements.clear();
	}

	void BufferLayout::calculateOffsetsAndStride() {
		uint32_t currentStride = 0;
		for (BufferElement& bufferElement : mBufferElements) {
			bufferElement.Offset = currentStride;
			size_t typeSize = getShaderDataTypeSize(bufferElement.Type);
			currentStride += typeSize;
		}
		mStride = currentStride;
	}

	VertexBuffer::VertexBuffer() 
		:	mRendererID(0)
	{
		glGenBuffers(1, &mRendererID);
		glBindBuffer(GL_ARRAY_BUFFER, mRendererID);
	}

	VertexBuffer::~VertexBuffer() {
		glDeleteBuffers(1, &mRendererID);
	}

	void VertexBuffer::bind() {
		glBindBuffer(GL_ARRAY_BUFFER, mRendererID);
	}

	void VertexBuffer::unbind() {
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void VertexBuffer::setData(void* data, size_t size, VertexBufferDataUsage dataUsage)
	{
		glBufferData(GL_ARRAY_BUFFER, size, data, dataUsageToOpenGLDataUsage(dataUsage));
	}

	void* VertexBuffer::map()
	{
		return glMapBuffer(GL_ARRAY_BUFFER, GL_READ_WRITE);
	}

	void VertexBuffer::unMap()
	{
		glUnmapBuffer(GL_ARRAY_BUFFER);
	}

	

}