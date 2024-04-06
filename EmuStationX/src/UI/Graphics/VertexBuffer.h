#pragma once

#include <string>
#include <vector>
#include <memory>

namespace esx {

	enum class ShaderType {
		Float,
		Float2,
		Float4,
		Short2,
		UShort2,
		UByte1,
		UByte2,
		UByte3,
		UByte4,
	};

	struct BufferElement {
		std::string Name;
		ShaderType Type;
		uint64_t Offset;
		bool Normalized;

		BufferElement(const std::string& name, ShaderType type, bool normalized = false);
	};

	class BufferLayout {
	public:
		BufferLayout() : mStride(0) { }
		BufferLayout(std::initializer_list<BufferElement> list);
		~BufferLayout();

		std::vector<BufferElement>::iterator begin() { return mBufferElements.begin(); }
		std::vector<BufferElement>::iterator end() { return mBufferElements.end(); }

		std::vector<BufferElement>::const_iterator begin() const { return mBufferElements.begin(); }
		std::vector<BufferElement>::const_iterator end() const { return mBufferElements.end(); }

		uint32_t getStride() const { return mStride; }
	private:
		void calculateOffsetsAndStride();
	private:
		std::vector<BufferElement> mBufferElements;
		uint32_t mStride;
	};

	enum class VertexBufferDataUsage {
		Static,
		Dynamic
	};

	class VertexBuffer {
	public:
		VertexBuffer();
		~VertexBuffer();

		void bind();
		void unbind();

		void setData(void* data, size_t size, VertexBufferDataUsage dataUsage);
		void* map();
		void unMap();

		void setLayout(const BufferLayout& layout) { mLayout = layout; }
		const BufferLayout& getLayout() const { return mLayout; }

	private:
		uint32_t mRendererID;
		BufferLayout mLayout;
	};

}
