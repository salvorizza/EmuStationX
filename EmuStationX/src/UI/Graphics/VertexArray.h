#pragma once

#include <memory>
#include <vector>

#include "VertexBuffer.h"
#include "IndexBuffer.h"

namespace esx {

	class VertexArray {
	public:
		VertexArray();
		~VertexArray();

		void bind();
		void addVertexBuffer(const std::shared_ptr<VertexBuffer>& vbo);
		void unbind();

		void setIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer) {
			indexBuffer->bind();
			mIndexBuffer = indexBuffer;
		}

		const std::shared_ptr<IndexBuffer>& getIndexBuffer() const { return mIndexBuffer; }

	private:
		uint32_t mRendererID;
		std::vector<std::shared_ptr<VertexBuffer>> mVertexBuffers;
		std::shared_ptr<IndexBuffer> mIndexBuffer;
	};

}