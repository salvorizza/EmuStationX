#include "UI/Graphics/BatchRenderer.h"


#include <glad/glad.h>

namespace esx {



	BatchRenderer::BatchRenderer()
		:	mTriVerticesBase(MAX_TRIS)
	{
		mTriVBO = MakeShared<VertexBuffer>();
		mTriVBO->setLayout({
			BufferElement("aPos", ShaderType::Short2),
			BufferElement("aUV", ShaderType::UShort2),
			BufferElement("aColor", ShaderType::UByte3),
			BufferElement("aTextured", ShaderType::UByte1),
			BufferElement("aClutUV", ShaderType::UShort2),
			BufferElement("aBPP", ShaderType::UByte1),
			BufferElement("aSemiTransparency", ShaderType::UByte1)
		});
		mTriVBO->setData(nullptr, TRI_BUFFER_SIZE, VertexBufferDataUsage::Dynamic);

		mTriVAO = MakeShared<VertexArray>();
		mTriVAO->addVertexBuffer(mTriVBO);
		mTriVAO->unbind();
		mTriVBO->unbind();

		U32 flags = BufferMode::Write | BufferMode::Persistent;

		mPBO16 = MakeShared<PixelBuffer>();
		mPBO16->setData(nullptr, 1024 * 512 * sizeof(U16), flags);
		mPBO16->unbind();
		mTexture16 = MakeShared<Texture2D>(0);
		mTexture16->setData(nullptr, 1024, 512, InternalFormat::R16, DataType::UnsignedShort, DataFormat::RED);
		mPBO16->bind();
		mPixels16 = (U16*)mPBO16->mapBufferRange();
		mPBO16->unbind();
		mTexture16->unbind();


		mPBO8 = MakeShared<PixelBuffer>();
		mPBO8->setData(nullptr, 1024 * 512 * 2, flags);
		mPBO8->unbind();
		mTexture8 = MakeShared<Texture2D>(1);
		mTexture8->setData(nullptr, 2048, 512, InternalFormat::R8, DataType::UnsignedByte, DataFormat::RED);
		mPBO8->bind();
		mPixels8 = (U8*)mPBO8->mapBufferRange();
		mPBO8->unbind();
		mTexture8->unbind();


		mPBO4 = MakeShared<PixelBuffer>();
		mPBO4->setData(nullptr, 1024 * 512 * 4, flags);
		mPBO4->unbind();
		mTexture4 = MakeShared<Texture2D>(2);
		mTexture4->setData(nullptr, 4096, 512, InternalFormat::R8, DataType::UnsignedByte, DataFormat::RED);
		mPBO4->bind();
		mPixels4 = (U8*)mPBO4->mapBufferRange();
		mPBO4->unbind();
		mTexture4->unbind();

		mFBO = MakeShared<FrameBuffer>(640, 480);


		mShader = Shader::LoadFromFile("commons/shaders/shader.vert","commons/shaders/shader.frag");
	}

	void BatchRenderer::Begin()
	{
		mTriCurrentVertex = mTriVerticesBase.begin();
	}

	void BatchRenderer::end()
	{
		Flush();
	}

	void BatchRenderer::Flush()
	{
		mFBO->bind();
		glViewport(0, 0, mFBO->width(), mFBO->height());

		mTexture16->bind();
		mTexture16->copy(mPBO16);

		mTexture8->bind();
		mTexture8->copy(mPBO8);

		mTexture4->bind();
		mTexture4->copy(mPBO4);

		mShader->start();
		mTexture16->bind();
		mTexture8->bind();
		mTexture4->bind();

		glActiveTexture(GL_TEXTURE0 + 3);
		glBindTexture(GL_TEXTURE_2D, mFBO->getColorAttachment()->getRendererID());


		mShader->uploadUniform("uOffset", mDrawOffset);
		mShader->uploadUniform("uTopLeft", mDrawTopLeft);
		mShader->uploadUniform("uBottomRight", mDrawBottomRight);

		ptrdiff_t numIndices = std::distance(mTriVerticesBase.begin(), mTriCurrentVertex);
		if (numIndices) {
			mTriVBO->bind();
			mTriVBO->setData(mTriVerticesBase.data(), numIndices * sizeof(PolygonVertex), VertexBufferDataUsage::Dynamic);

			mTriVAO->bind();
			glDrawArrays(GL_TRIANGLES, 0, numIndices);
			glFinish();
		}

		mFBO->unbind();
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
		ESX_CORE_LOG_TRACE("DrawPolygon");

		for (U64 i = 0; i < 3; i++) {
			const PolygonVertex& vertex = vertices[i];

			ESX_CORE_LOG_TRACE("[{},{}],[{},{},{}],[{},{}],[{},{}]", vertex.vertex.x, vertex.vertex.y, vertex.color.r, vertex.color.g, vertex.color.b, vertex.uv.u, vertex.uv.v, vertex.clutUV.u, vertex.clutUV.v);

			*mTriCurrentVertex = vertex;
			mTriCurrentVertex++;
		}

		if (vertices.size() == 4) {
			for (U64 i = 1; i < 4; i++) {
				const PolygonVertex& vertex = vertices[i];

				ESX_CORE_LOG_TRACE("[{},{}],[{},{},{}],[{},{}],[{},{}]", vertex.vertex.x, vertex.vertex.y, vertex.color.r, vertex.color.g, vertex.color.b, vertex.uv.u, vertex.uv.v, vertex.clutUV.u, vertex.clutUV.v);
				*mTriCurrentVertex = vertex;
				mTriCurrentVertex++;
			}
		}

	}

	void BatchRenderer::DrawRectangle(const Vertex& topLeft, U16 width, U16 height, const Color& color)
	{
		ESX_CORE_LOG_TRACE("DrawRectangle");

		//Triangle 1,2,3
		mTriCurrentVertex->vertex = Vertex(topLeft.x + width, topLeft.y + height);
		mTriCurrentVertex->color = color;
		mTriCurrentVertex->uv = UV(0,0);
		mTriCurrentVertex++;

		mTriCurrentVertex->vertex = Vertex(topLeft.x, topLeft.y + height);
		mTriCurrentVertex->color = color;
		mTriCurrentVertex->uv = UV(0, 0);
		mTriCurrentVertex++;

		mTriCurrentVertex->vertex = Vertex(topLeft.x + width, topLeft.y);
		mTriCurrentVertex->color = color;
		mTriCurrentVertex->uv = UV(0, 0);
		mTriCurrentVertex++;


		//Triangle 2,3,4
		mTriCurrentVertex->vertex = Vertex(topLeft.x, topLeft.y + height);
		mTriCurrentVertex->color = color;
		mTriCurrentVertex->uv = UV(0, 0);
		mTriCurrentVertex++;

		mTriCurrentVertex->vertex = Vertex(topLeft.x + width, topLeft.y);
		mTriCurrentVertex->color = color;
		mTriCurrentVertex->uv = UV(0, 0);
		mTriCurrentVertex++;

		mTriCurrentVertex->vertex = Vertex(topLeft.x, topLeft.y);
		mTriCurrentVertex->color = color;
		mTriCurrentVertex->uv = UV(0, 0);
		mTriCurrentVertex++;
	}

	void BatchRenderer::VRAMWrite(U16 x, U16 y, U16 data)
	{
		U64 index = (y * 1024) + x;

		mPixels16[index] = data;

		mPixels8[index * 2 + 0] = (U8)data;
		mPixels8[index * 2 + 1] = (U8)(data >> 8);

		mPixels4[index * 4 + 0] = (U8)data & 0xF;
		mPixels4[index * 4 + 1] = (U8)(data >> 4) & 0xF;
		mPixels4[index * 4 + 2] = (U8)(data >> 8) & 0xF;
		mPixels4[index * 4 + 3] = (U8)(data >> 12) & 0xF;
	}

	U16 BatchRenderer::VRAMRead(U16 x, U16 y)
	{
		U64 index = (y * 1024) + x;
		return mPixels16[index];
	}

}