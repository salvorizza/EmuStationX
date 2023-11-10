#pragma once

#include <cstdint>

namespace esx {

	class IndexBuffer {
	public:
		IndexBuffer(uint32_t* indices, uint32_t num);
		~IndexBuffer();

		void bind();
		void unbind();

		uint32_t getNumIndices() const { return mNumIndices; }

	private:
		uint32_t mRendererID;
		uint32_t mNumIndices;
	};

}
