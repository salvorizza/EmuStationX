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

		mFBO = MakeShared<FrameBuffer>(1024, 512);
		SharedPtr<Texture2D> colorAttachment = MakeShared<Texture2D>(0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		colorAttachment->setData(nullptr, 1024, 512, InternalFormat::RGBA8, DataType::UnsignedByte, DataFormat::RGBA);
		mFBO->setColorAttachment(colorAttachment);
		mFBO->init();

		mVRAM24.resize(1024 * 512 * 4);

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

			mShader->start();
			mFBO->getColorAttachment()->bind();

			mTriVBO->bind();
			mTriVBO->setData(mTriVerticesBase.data(), numIndices * sizeof(PolygonVertex), VertexBufferDataUsage::Dynamic);

			mTriVAO->bind();
			glDrawArrays(GL_TRIANGLES, 0, (GLsizei)numIndices);
			glFinish();

			mTriVAO->unbind();
			mTriVBO->unbind();
			mFBO->getColorAttachment()->unbind();
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

			//ESX_CORE_LOG_TRACE(" Vertex({},{}),Color({},{},{}),Textured({})", vertex.vertex.x, vertex.vertex.y, vertex.color.r, vertex.color.g, vertex.color.b, vertex.textured);
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
		U8 pixels[4] = {
			((data >> 0) & 0x1F) << 3,
			((data >> 5) & 0x1F) << 3,
			((data >> 10) & 0x1F) << 3,
			(data >> 15) != 0 ? 255 : 0
		};

		mFBO->getColorAttachment()->bind();
		mFBO->getColorAttachment()->setPixel(x, (511-y), &pixels);
		mFBO->getColorAttachment()->unbind();
	}

	U16 BatchRenderer::VRAMRead(U16 x, U16 y)
	{
		Flush();
		Begin();

		U64 index = ((511-y) * 1024) + x;

		U8 r = mVRAM24[(index * 4) + 0] >> 3;
		U8 g = mVRAM24[(index * 4) + 1] >> 3;
		U8 b = mVRAM24[(index * 4) + 2] >> 3;
		U8 a = (mVRAM24[(index * 4) + 3] != 0) ? 1 : 0;

		U16 data = (a << 15) | (b << 10) | (g << 5) | r;

		return data;
	}

	void BatchRenderer::Reset()
	{
		std::fill(mTriVerticesBase.begin(), mTriVerticesBase.end(), PolygonVertex());

		mFBO->bind();
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		mFBO->unbind();

		mVRAM24.resize(1024 * 512 * 4);
		std::fill(mVRAM24.begin(), mVRAM24.end(), 0x00);

		mDrawTopLeft = glm::uvec2(0, 0);
		mDrawBottomRight = glm::uvec2(640, 240);

		glEnable(GL_SCISSOR_TEST);
	}

}