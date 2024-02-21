#include "UI/Graphics/BatchRenderer.h"


#include <glad/glad.h>

namespace esx {



	BatchRenderer::BatchRenderer()
		:	mVerticesBase(nullptr),
			mCurrentVertex(nullptr),
			mNumIndices(0)
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
			BufferElement("aPosition", ShaderType::Float2),
			BufferElement("aUV", ShaderType::Float2),
			BufferElement("aColor", ShaderType::Uint4, true)
		});
		mVerticesBase = new QuadVertex[MAX_QUADS];
		mVBO->setData(NULL, QUAD_BUFFER_SIZE, VertexBufferDataUsage::Dynamic);

		mVAO = std::make_shared<VertexArray>();
		mVAO->addVertexBuffer(mVBO);
		mVAO->setIndexBuffer(mIBO);
		mVAO->unbind();
		mVBO->unbind();
		mIBO->unbind();

		mTriVBO = std::make_shared<VertexBuffer>();
		mTriVBO->setLayout({
			BufferElement("aPosition", ShaderType::Float2),
			BufferElement("aUV", ShaderType::Float2),
			BufferElement("aColor", ShaderType::Uint4, true)
			});
		mTriVerticesBase = new QuadVertex[MAX_TRIS];
		mTriVBO->setData(NULL, TRI_BUFFER_SIZE, VertexBufferDataUsage::Dynamic);

		mTriVAO = std::make_shared<VertexArray>();
		mTriVAO->addVertexBuffer(mTriVBO);
		mTriVAO->unbind();
		mTriVBO->unbind();

		mShader = Shader::LoadFromFile("commons/shaders/quadShader.vert","commons/shaders/quadShader.frag");
	}

	BatchRenderer::~BatchRenderer()
	{

	}

	void BatchRenderer::begin(const glm::mat4& projMatrix, float borderWidth, float aspectRatio)
	{
		mProjectionMatrix = projMatrix;
		mBorderWidth = borderWidth;
		mAspectRatio = aspectRatio;

		mCurrentVertex = mVerticesBase;
		mNumIndices = 0;

		mTriCurrentVertex = mTriVerticesBase;
		mTriNumIndices = 0;
	}

	void BatchRenderer::end()
	{
		flush();
	}

	void BatchRenderer::flush()
	{
		mShader->start();
		mShader->uploadUniform("uProjectionMatrix", mProjectionMatrix);
		mShader->uploadUniform("uBorderWidth", mBorderWidth);
		mShader->uploadUniform("uAspectRatio", mAspectRatio);

		if (mNumIndices) {
			U32 dataSize = (U32)((U8*)mCurrentVertex - (U8*)mVerticesBase);
			mVBO->bind();
			mVBO->setData(mVerticesBase, dataSize, VertexBufferDataUsage::Dynamic);

			mVAO->bind();
			glDrawElements(GL_TRIANGLES, mNumIndices, GL_UNSIGNED_INT, NULL);
		}

		if (mTriNumIndices) {
			U32 dataSize = (U32)((U8*)mTriCurrentVertex - (U8*)mTriVerticesBase);
			mTriVBO->bind();
			mTriVBO->setData(mTriVerticesBase, dataSize, VertexBufferDataUsage::Dynamic);

			mTriVAO->bind();
			glDrawArrays(GL_TRIANGLES, 0, mTriNumIndices);
		}
	}

	void BatchRenderer::drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
	{
		if (mNumIndices == QUAD_MAX_NUM_INDICES) {
			flush();
			begin(mProjectionMatrix, mBorderWidth, mAspectRatio);
		}

		uint32_t a = (uint32_t)(color.a * 255);
		uint32_t b = (uint32_t)(color.b * 255);
		uint32_t g = (uint32_t)(color.g * 255);
		uint32_t r = (uint32_t)(color.r * 255);

		uint32_t u32color = a << 24 | b << 16 | g << 8 | r;


		mCurrentVertex->Position = position;
		mCurrentVertex->Color = u32color;
		mCurrentVertex->UV = glm::vec2(0.0, 1.0);
		mCurrentVertex++;

		mCurrentVertex->Position = position + glm::vec2(size.x, 0);
		mCurrentVertex->Color = u32color;
		mCurrentVertex->UV = glm::vec2(1.0, 1.0);
		mCurrentVertex++;

		mCurrentVertex->Position = position + glm::vec2(size.x, size.y);
		mCurrentVertex->Color = u32color;
		mCurrentVertex->UV = glm::vec2(1.0, 0.0);
		mCurrentVertex++;

		mCurrentVertex->Position = position + glm::vec2(0, size.y);
		mCurrentVertex->Color = u32color;
		mCurrentVertex->UV = glm::vec2(0.0, 0.0);
		mCurrentVertex++;

		mNumIndices += 6;
	}

	void BatchRenderer::DrawPolygon(const Vector<PolygonVertex>& vertices)
	{
		if (vertices.size() == 4) {
			if (mNumIndices == QUAD_MAX_NUM_INDICES) {
				flush();
				begin(mProjectionMatrix, mBorderWidth, mAspectRatio);
			}

			for (const PolygonVertex& vertex : vertices) {
				uint32_t a = (uint32_t)(255);
				uint32_t b = (uint32_t)(vertex.color.b);
				uint32_t g = (uint32_t)(vertex.color.g);
				uint32_t r = (uint32_t)(vertex.color.r);
				uint32_t u32color = a << 24 | b << 16 | g << 8 | r;

				mCurrentVertex->Position = glm::vec2(vertex.vertex.x, vertex.vertex.y);
				mCurrentVertex->Color = u32color;
				mCurrentVertex->UV = glm::vec2(0.0, 0.0);
				mCurrentVertex++;
			}

			mNumIndices += 6;
		}
		else {
			if (mTriNumIndices == TRI_MAX_NUM_INDICES) {
				flush();
				begin(mProjectionMatrix, mBorderWidth, mAspectRatio);
			}

			for (const PolygonVertex& vertex : vertices) {
				uint32_t a = (uint32_t)(255);
				uint32_t b = (uint32_t)(vertex.color.b);
				uint32_t g = (uint32_t)(vertex.color.g);
				uint32_t r = (uint32_t)(vertex.color.r);
				uint32_t u32color = a << 24 | b << 16 | g << 8 | r;

				mTriCurrentVertex->Position = glm::vec2(vertex.vertex.x, vertex.vertex.y);
				mTriCurrentVertex->Color = u32color;
				mTriCurrentVertex->UV = glm::vec2(0.0, 0.0);
				mTriCurrentVertex++;
			}

			mTriNumIndices += 3;
		}
	}

	void BatchRenderer::DrawRectangle(const Vertex& topLeft, U16 width, U16 height, const Color& color)
	{
	}

}