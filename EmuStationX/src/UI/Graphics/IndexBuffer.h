#pragma once

#include <cstdint>

#include "VertexBuffer.h"

namespace esx {

	class IndexBuffer {
	public:
		IndexBuffer();
		IndexBuffer(uint32_t* indices, uint32_t num);
		~IndexBuffer();

		void bind();
		void unbind();

		void setData(void* data, size_t size, VertexBufferDataUsage dataUsage);
		void copyData(void* data, size_t size);

		uint32_t getNumIndices() const { return mNumIndices; }

	private:
		uint32_t mRendererID = 0;
		uint32_t mNumIndices = 0;
	};

}
