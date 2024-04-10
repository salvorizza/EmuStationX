#include "UI/Graphics/BatchRenderer.h"


#include <glad/glad.h>

namespace esx {



	BatchRenderer::BatchRenderer()
		:	mTriVerticesBase(TRI_MAX_VERTICES)
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

		mFBO = MakeShared<FrameBuffer>(1024, 512);
		SharedPtr<Texture2D> colorAttachment = MakeShared<Texture2D>(3);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		colorAttachment->setData(nullptr, 1024, 512, InternalFormat::RGB8, DataType::UnsignedByte, DataFormat::RGB);
		mFBO->setColorAttachment(colorAttachment);
		mFBO->init();

		mVRAM24.resize(1024 * 512 * 3);

		mShader = Shader::LoadFromFile("commons/shaders/shader.vert","commons/shaders/shader.frag");

		mDrawTopLeft = glm::uvec2(0, 0);
		mDrawBottomRight = glm::uvec2(640, 240);

		glEnable(GL_SCISSOR_TEST);

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
		ptrdiff_t numIndices = std::distance(mTriVerticesBase.begin(), mTriCurrentVertex);
		if (numIndices > 0) {
			mFBO->bind();
			glViewport(0, 0, mFBO->width(), mFBO->height());
			glScissor(mDrawTopLeft.x, 512 - mDrawBottomRight.y, mDrawBottomRight.x - mDrawTopLeft.x, mDrawBottomRight.y - mDrawTopLeft.y);

			mTexture16->bind();
			mTexture16->copy(mPBO16);
			mPBO16->unbind();

			mTexture8->bind();
			mTexture8->copy(mPBO8);
			mPBO8->unbind();

			mTexture4->bind();
			mTexture4->copy(mPBO4);
			mPBO4->unbind();

			mShader->start();
			mTexture16->bind();
			mTexture8->bind();
			mTexture4->bind();
			mFBO->getColorAttachment()->bind();

			/*mShader->uploadUniform("uTopLeft", mDrawTopLeft);
			mShader->uploadUniform("uBottomRight", mDrawBottomRight);*/

			mTriVBO->bind();
			mTriVBO->setData(mTriVerticesBase.data(), numIndices * sizeof(PolygonVertex), VertexBufferDataUsage::Dynamic);

			mTriVAO->bind();
			glDrawArrays(GL_TRIANGLES, 0, (GLsizei)numIndices);
			glFinish();

			mTriVAO->unbind();
			mTriVBO->unbind();
			mFBO->getColorAttachment()->unbind();
			mTexture4->unbind();
			mTexture8->unbind();
			mTexture16->unbind();
			mShader->stop();
			mFBO->unbind();

			void* pixels = mVRAM24.data();
			mFBO->getColorAttachment()->bind();
			mFBO->getColorAttachment()->getPixels(&pixels);
			mFBO->getColorAttachment()->unbind();
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

	void BatchRenderer::DrawPolygon(Vector<PolygonVertex>& vertices)
	{
		ptrdiff_t numIndices = std::distance(mTriVerticesBase.begin(), mTriCurrentVertex);
		if (numIndices == TRI_MAX_VERTICES) {
			Flush();
			Begin();
		}

		for (PolygonVertex& vertex : vertices) {
			vertex.vertex.x += mDrawOffset.x;
			vertex.vertex.y += mDrawOffset.y;
		}

		for (U64 i = 0; i < 3; i++) {
			const PolygonVertex& vertex = vertices[i];

			*mTriCurrentVertex = vertex;
			mTriCurrentVertex++;
		}

		if (vertices.size() == 4) {
			for (U64 i = 1; i < 4; i++) {
				const PolygonVertex& vertex = vertices[i];

				*mTriCurrentVertex = vertex;
				mTriCurrentVertex++;
			}
		}

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


		U8 pixels[3] = {
			((data << 3) & 0xf8),
			((data >> 2) & 0xf8),
			((data >> 7) & 0xf8),
		};
		mFBO->getColorAttachment()->bind();
		mFBO->getColorAttachment()->setPixel(x, 511 - y, &pixels);
		mFBO->getColorAttachment()->unbind();
	}

	U16 BatchRenderer::VRAMRead(U16 x, U16 y)
	{
		U64 index = ((511-y) * 1024) + x;

		U8 r = mVRAM24[(index * 3) + 0] >> 3;
		U8 g = mVRAM24[(index * 3) + 1] >> 3;
		U8 b = mVRAM24[(index * 3) + 2] >> 3;

		U16 data = (b << 10) | (g << 5) | r;

		return data;
	}

}