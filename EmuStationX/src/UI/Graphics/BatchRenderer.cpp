#include "UI/Graphics/BatchRenderer.h"


#include <glad/glad.h>

namespace esx {



	BatchRenderer::BatchRenderer()
		:	mVerticesBase(MAX_QUADS),
			mTriVerticesBase(MAX_TRIS)
	{
		Vector<U32> indices(QUAD_MAX_NUM_INDICES);
		for (uint32_t i = 0; i < MAX_QUADS; i++) {
			indices[(i * 6) + 0] = (i * 4) + 0;
			indices[(i * 6) + 1] = (i * 4) + 1;
			indices[(i * 6) + 2] = (i * 4) + 2;
			indices[(i * 6) + 3] = (i * 4) + 1;
			indices[(i * 6) + 4] = (i * 4) + 2;
			indices[(i * 6) + 5] = (i * 4) + 3;
		}

		mIBO = std::make_shared<IndexBuffer>(indices.data(), QUAD_MAX_NUM_INDICES);
		mVBO = std::make_shared<VertexBuffer>();
		mVBO->setLayout({
			BufferElement("aPos", ShaderType::Short2),
			BufferElement("aUV", ShaderType::UByte2),
			BufferElement("aColor", ShaderType::UByte3)
		});
		mVBO->setData(nullptr, QUAD_BUFFER_SIZE, VertexBufferDataUsage::Dynamic);

		mVAO = std::make_shared<VertexArray>();
		mVAO->addVertexBuffer(mVBO);
		mVAO->setIndexBuffer(mIBO);
		mVAO->unbind();
		mVBO->unbind();
		mIBO->unbind();

		mTriVBO = std::make_shared<VertexBuffer>();
		mTriVBO->setLayout({
			BufferElement("aPos", ShaderType::Short2),
			BufferElement("aUV", ShaderType::UByte2),
			BufferElement("aColor", ShaderType::UByte3)
		});
		mTriVBO->setData(nullptr, TRI_BUFFER_SIZE, VertexBufferDataUsage::Dynamic);

		mTriVAO = std::make_shared<VertexArray>();
		mTriVAO->addVertexBuffer(mTriVBO);
		mTriVAO->unbind();
		mTriVBO->unbind();

		mShader = Shader::LoadFromFile("commons/shaders/shader.vert","commons/shaders/shader.frag");
	}

	void BatchRenderer::Begin()
	{
		mCurrentVertex = mVerticesBase.begin();
		mNumIndices = 0;

		mTriCurrentVertex = mTriVerticesBase.begin();
		mTriNumIndices = 0;
	}

	void BatchRenderer::end()
	{
		Flush();
	}

	void BatchRenderer::Flush()
	{
		mShader->start();
		mShader->uploadUniform("uOffset", mDrawOffset);
		mShader->uploadUniform("uTopLeft", mDrawTopLeft);
		mShader->uploadUniform("uBottomRight", mDrawBottomRight);

		if (mNumIndices) {
			ptrdiff_t dataSize = std::distance(mVerticesBase.begin(), mCurrentVertex) * sizeof(PolygonVertex);
			mVBO->bind();
			mVBO->setData(mVerticesBase.data(), dataSize, VertexBufferDataUsage::Dynamic);

			mVAO->bind();
			glDrawElements(GL_TRIANGLES, mNumIndices, GL_UNSIGNED_INT, nullptr);
			glFinish();
		}

		if (mTriNumIndices) {
			ptrdiff_t dataSize = std::distance(mTriVerticesBase.begin(), mTriCurrentVertex) * sizeof(PolygonVertex);
			mTriVBO->bind();
			mTriVBO->setData(mTriVerticesBase.data(), dataSize, VertexBufferDataUsage::Dynamic);

			mTriVAO->bind();
			glDrawArrays(GL_TRIANGLES, 0, mTriNumIndices);
			glFinish();
		}
	}

	void BatchRenderer::SetDrawOffset(I16 offsetX, I16 offsetY)
	{
		mDrawOffset.x = offsetX;
		mDrawOffset.y = offsetY;
	}

	void BatchRenderer::SetDrawTopLeft(U16 x, U16 y)
	{
		mDrawTopLeft.x = x;
		mDrawTopLeft.y = y;
	}

	void BatchRenderer::SetDrawBottomRight(U16 x, U16 y)
	{
		mDrawBottomRight.x = x;
		mDrawBottomRight.y = y;
	}

	void BatchRenderer::DrawPolygon(const Vector<PolygonVertex>& vertices)
	{
		if (vertices.size() == 4) {
			if (mNumIndices == QUAD_MAX_NUM_INDICES) {
				Flush();
				Begin();
			}

			for (const PolygonVertex& vertex : vertices) {
				*mCurrentVertex = vertex;
				mCurrentVertex++;
			}

			mNumIndices += 6;
		}
		else {
			if (mTriNumIndices == TRI_MAX_NUM_INDICES) {
				Flush();
				Begin();
			}

			for (const PolygonVertex& vertex : vertices) {
				*mTriCurrentVertex = vertex;
				mTriCurrentVertex++;
			}

			mTriNumIndices += 3;
		}
	}

	void BatchRenderer::DrawRectangle(const Vertex& topLeft, U16 width, U16 height, const Color& color)
	{
		if (mNumIndices == QUAD_MAX_NUM_INDICES) {
			Flush();
			Begin();
		}

		mCurrentVertex->vertex = Vertex(topLeft.x + width, topLeft.y + height);
		mCurrentVertex->color = color;
		mCurrentVertex->uv = UV(0,0);
		mCurrentVertex++;

		mCurrentVertex->vertex = Vertex(topLeft.x, topLeft.y + height);
		mCurrentVertex->color = color;
		mCurrentVertex->uv = UV(0, 0);
		mCurrentVertex++;

		mCurrentVertex->vertex = Vertex(topLeft.x + width, topLeft.y);
		mCurrentVertex->color = color;
		mCurrentVertex->uv = UV(0, 0);
		mCurrentVertex++;

		mCurrentVertex->vertex = Vertex(topLeft.x, topLeft.y);
		mCurrentVertex->color = color;
		mCurrentVertex->uv = UV(0, 0);
		mCurrentVertex++;

		mNumIndices += 6;
	}

}