#include "UI/Graphics/BatchRenderer.h"


#include <glad/glad.h>

#include "Utils/Geometry.h"

namespace esx {



	BatchRenderer::BatchRenderer()
		:	mTriVerticesBase(TRI_MAX_VERTICES),
			mLineStripVerticesBase(MAX_LINE_STRIP_VERTICES),
			mLineStripIndicesBase(MAX_LINE_STRIP_VERTICES)
	{
		BufferLayout defaultLayout = {
			BufferElement("aPos", ShaderType::Short2),
			BufferElement("aUV", ShaderType::UShort2),
			BufferElement("aColor", ShaderType::UByte3),
			BufferElement("aTextured", ShaderType::UByte1),
			BufferElement("aClutUV", ShaderType::UShort2),
			BufferElement("aBPP", ShaderType::UByte1),
			BufferElement("aSemiTransparency", ShaderType::UByte1),
			BufferElement("aDither", ShaderType::UByte1),
			BufferElement("aRawTexture", ShaderType::UByte1)
		};

		mTriVBO = MakeShared<VertexBuffer>();
		mTriVBO->setLayout(defaultLayout);
		mTriVBO->setData(nullptr, TRI_BUFFER_SIZE, VertexBufferDataUsage::Dynamic);
		mTriVAO = MakeShared<VertexArray>();
		mTriVAO->addVertexBuffer(mTriVBO);
		mTriVAO->unbind();
		mTriVBO->unbind();

		mLineStripVBO = MakeShared<VertexBuffer>();
		mLineStripVBO->setLayout(defaultLayout);
		mLineStripVBO->setData(nullptr, LINE_STRIP_BUFFER_SIZE, VertexBufferDataUsage::Dynamic);
		mLineStripIBO = MakeShared<IndexBuffer>();
		mLineStripIBO->setData(nullptr, MAX_LINE_STRIP_VERTICES * sizeof(U32), VertexBufferDataUsage::Dynamic);
		mLineStripVAO = MakeShared<VertexArray>();
		mLineStripVAO->addVertexBuffer(mLineStripVBO);
		mLineStripVAO->setIndexBuffer(mLineStripIBO);
		mLineStripVAO->unbind();
		mLineStripVBO->unbind();

		
		
		mFBO16 = MakeShared<FrameBuffer>(1024, 512);
		mTexture16 = MakeShared<Texture2D>(0);
		mTexture16->setData(nullptr, 1024, 512, InternalFormat::RGB5_A1, DataType::UnsignedShort1_555, DataFormat::RGBA);
		mFBO16->setColorAttachment(mTexture16);
		mFBO16->init();

		mFBO24 = MakeShared<FrameBuffer>(1024, 512);
		mTexture24 = MakeShared<Texture2D>(1);
		mTexture24->setData(nullptr, 682, 512, InternalFormat::RGB8, DataType::UnsignedByte, DataFormat::RGB);
		mFBO24->setColorAttachment(mTexture24);
		mFBO24->init();

		/*mPBO24Up = MakeShared<PixelBuffer>();
		mPBO24Up->setData(nullptr, 682 * 512 * sizeof(U8) * 3, BufferMode::Write);
		mPBO24Up->unbind();*/

		mPBO16Up = MakeShared<PixelBuffer>();
		mPBO16Up->setData(nullptr, 1024 * 512 * sizeof(VRAMColor), BufferMode::Write);
		mPBO16Up->unbind();

		mVRAM16.resize(1024 * 512);

		mShader = Shader::LoadFromFile("commons/shaders/shader.vert","commons/shaders/shader.frag");

		mDrawTopLeft = glm::uvec2(0, 0);
		mDrawBottomRight = glm::uvec2(640, 240);

		glEnable(GL_SCISSOR_TEST);
	}

	void BatchRenderer::Begin()
	{
		mTriCurrentVertex = mTriVerticesBase.begin();
		mLineStripCurrentVertex = mLineStripVerticesBase.begin();
		mLineStripCurrentIndex = mLineStripIndicesBase.begin();
	}

	void BatchRenderer::end()
	{
		FlushVRAMWrites();
		Flush();
	}

	void BatchRenderer::Flush()
	{
		ptrdiff_t numTriIndices = std::distance(mTriVerticesBase.begin(), mTriCurrentVertex);
		ptrdiff_t numLineStripIndices = std::distance(mLineStripVerticesBase.begin(), mLineStripCurrentVertex);


		if (numTriIndices > 0 || numLineStripIndices > 0) {
			mFBO16->bind();

			glViewport(0, 0, mFBO16->width(), mFBO16->height());
			glScissor(mDrawTopLeft.x, 511 - mDrawBottomRight.y, (mDrawBottomRight.x - mDrawTopLeft.x) + 1, (mDrawBottomRight.y - mDrawTopLeft.y) + 1);

			mShader->start();

			mShader->uploadUniform("uCheckMask", mCheckMask);
			mShader->uploadUniform("uForceAlpha", mForceAlpha);

			mFBO16->getColorAttachment()->bind();

			if (numTriIndices > 0) {
				mTriVBO->bind();
				mTriVBO->copyData(mTriVerticesBase.data(), numTriIndices * sizeof(PolygonVertex));
				mTriVAO->bind();
				glDrawArrays(GL_TRIANGLES, 0, (GLsizei)numTriIndices);
			}

			if (numLineStripIndices > 0) {

				glEnable(GL_PRIMITIVE_RESTART);
				glLineWidth(1);
				glDisable(GL_LINE_SMOOTH);
				glPrimitiveRestartIndex(-1);

				mLineStripVBO->bind();
				mLineStripVBO->copyData(mLineStripVerticesBase.data(), numLineStripIndices * sizeof(PolygonVertex));
				mLineStripIBO->bind();
				mLineStripIBO->copyData(mLineStripIndicesBase.data(), numLineStripIndices * sizeof(U32));

				mLineStripVAO->bind();
				glDrawElements(GL_LINE_STRIP, numLineStripIndices, GL_UNSIGNED_INT, nullptr);


				glDisable(GL_PRIMITIVE_RESTART);
			}

			mFBO16->getColorAttachment()->unbind();
			mShader->stop();
			mFBO16->unbind();

			mRefreshVRAMData = ESX_TRUE;
		}
	}

	void BatchRenderer::SetDrawOffset(I16 offsetX, I16 offsetY)
	{
		mDrawOffset.x = offsetX;
		mDrawOffset.y = offsetY;
		//ESX_CORE_LOG_TRACE("BatchRenderer::SetDrawOffset({},{})", mDrawOffset.x, mDrawOffset.y);
	}

	void BatchRenderer::SetDrawTopLeft(U16 x, U16 y)
	{
		FlushVRAMWrites();
		Flush();
		Begin();

		mDrawTopLeft.x = x;
		mDrawTopLeft.y = y;

		//ESX_CORE_LOG_TRACE("BatchRenderer::SetDrawTopLeft({},{})", mDrawTopLeft.x, mDrawTopLeft.y);
	}

	void BatchRenderer::SetDrawBottomRight(U16 x, U16 y)
	{
		FlushVRAMWrites();
		Flush();
		Begin();

		mDrawBottomRight.x = x;
		mDrawBottomRight.y = y;


		//ESX_CORE_LOG_TRACE("BatchRenderer::SetDrawBottomRight({},{})", mDrawBottomRight.x, mDrawBottomRight.y);
	}

	void BatchRenderer::SetForceAlpha(BIT value)
	{
		FlushVRAMWrites();
		Flush();
		Begin();

		mForceAlpha = value;
	}

	void BatchRenderer::SetCheckMask(BIT value)
	{
		FlushVRAMWrites();
		Flush();
		Begin();

		mCheckMask = value;
	}

	void BatchRenderer::SetDisplayMode24(BIT value)
	{
		m24Bit = value;
	}

	void BatchRenderer::Clear(U16 x, U16 y, U16 w, U16 h, Color& color)
	{
		FlushVRAMWrites();
		Flush();
		Begin();

		mFBO16->bind();
		glScissor(x, 511 - y - (h - 1), w, h);
		glClearColor((color.r >> 3) / 31.0, (color.g >> 3) / 31.0, (color.b >> 3) / 31.0, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		mFBO16->unbind();

		mRefreshVRAMData = ESX_TRUE;
	}

	void BatchRenderer::DrawPolygon(Array<PolygonVertex, 4>& vertices, U32 numVertices)
	{
		//ESX_CORE_LOG_TRACE("BatchRenderer::DrawPolygon");
		for (PolygonVertex& vertex : vertices) {
			vertex.vertex.x += mDrawOffset.x;
			vertex.vertex.y += mDrawOffset.y;

			//ESX_CORE_LOG_TRACE(" Vertex({},{}),Color({},{},{}),UV({},{}),ClutUV({},{}),Textured({})", vertex.vertex.x, vertex.vertex.y, vertex.color.r, vertex.color.g, vertex.color.b, vertex.uv.u, vertex.uv.v, vertex.clutUV.u,vertex.clutUV.v, vertex.textured);
		}

		ptrdiff_t numIndices = std::distance(mTriVerticesBase.begin(), mTriCurrentVertex);
		if ((numIndices + vertices.size()) >= TRI_MAX_VERTICES /* || (numIndices > 0 && vertices.at(0).semiTransparency != 255)*/) {
			FlushVRAMWrites();
			Flush();
			Begin();
		}

		if (numIndices > 0 && vertices.at(0).semiTransparency != 255) {
			BIT overlapped = ESX_FALSE;

			Vertex& triA_a = vertices.at(0).vertex;
			Vertex& triA_b = vertices.at(1).vertex;
			Vertex& triA_c = vertices.at(2).vertex;
			for (auto it = mTriVerticesBase.begin(); it < mTriCurrentVertex;) {
				Vertex& triB_a = it->vertex; it++;
				Vertex& triB_b = it->vertex; it++;
				Vertex& triB_c = it->vertex; it++;

				if (checkOverlap(triA_a, triA_b, triA_c, triB_a, triB_b, triB_c)) {
					overlapped = ESX_TRUE;
					break;
				}
			}

			if (!overlapped && numVertices == 4) {
				Vertex& triA_a = vertices.at(1).vertex;
				Vertex& triA_b = vertices.at(2).vertex;
				Vertex& triA_c = vertices.at(3).vertex;
				for (auto it = mTriVerticesBase.begin(); it < mTriCurrentVertex;) {
					Vertex& triB_a = it->vertex; it++;
					Vertex& triB_b = it->vertex; it++;
					Vertex& triB_c = it->vertex; it++;

					if (checkOverlap(triA_a, triA_b, triA_c, triB_a, triB_b, triB_c)) {
						overlapped = ESX_TRUE;
						break;
					}
				}
			}

			if (overlapped) {
				FlushVRAMWrites();
				Flush();
				Begin();
			}
		}

		for (U64 i = 0; i < 3; i++) {
			const PolygonVertex& vertex = vertices[i];

			*mTriCurrentVertex = vertex;
			mTriCurrentVertex++;
		}

		if (numVertices == 4) {
			for (U64 i = 1; i < 4; i++) {
				const PolygonVertex& vertex = vertices[i];

				*mTriCurrentVertex = vertex;
				mTriCurrentVertex++;
			}
		}
	}

	void BatchRenderer::DrawLineStrip(Vector<PolygonVertex>& vertices)
	{
		for (PolygonVertex& vertex : vertices) {
			vertex.vertex.x += mDrawOffset.x;
			vertex.vertex.y += mDrawOffset.y;
		}

		ptrdiff_t numIndices = std::distance(mLineStripVerticesBase.begin(), mLineStripCurrentVertex);
		if ((numIndices + vertices.size()) >= MAX_LINE_STRIP_VERTICES || (numIndices > 0 && vertices.at(0).semiTransparency != 255)) {
			FlushVRAMWrites();
			Flush();
			Begin();
		}

		for (U64 i = 0; i < vertices.size(); i++) {
			const PolygonVertex& vertex = vertices[i];

			*mLineStripCurrentIndex = std::distance(mLineStripVerticesBase.begin(), mLineStripCurrentVertex);
			mLineStripCurrentIndex++;

			*mLineStripCurrentVertex = vertex;
			mLineStripCurrentVertex++;
		}

		*mLineStripCurrentIndex = -1;
		mLineStripCurrentIndex++;
		mLineStripCurrentVertex++;
	}

	void BatchRenderer::VRAMWrite(U16 x, U16 y, U32 width, U32 height, const Vector<VRAMColor>& pixels)
	{
		Flush();
		Begin();

		if (mRefreshVRAMData) {
			refresh16BitData();
			mRefreshVRAMData = ESX_FALSE;
		}

		for (I32 yOff = 0; yOff < height; yOff++) {
			for (I32 xOff = 0; xOff < width; xOff++) {
				U16 xImage = x + xOff;
				U16 yImage = (511 - y - yOff);

				xImage &= 1023;
				yImage &= 511;

				VRAMColor color = pixels.at(yOff * width + xOff);

				if (mCheckMask && (mVRAM16[yImage * 1024 + xImage].data & 0x8000) == 0x8000) continue;
				if (mForceAlpha) color.data |= 0x8000;

				mVRAM16[yImage * 1024 + xImage] = color;
			}
		}

		mVRAMWritePending = ESX_TRUE;
	}

	void BatchRenderer::VRAMRead(U16 x, U16 y, U32 width, U32 height, Vector<VRAMColor>& pixels)
	{
		Flush();
		Begin();

		if (mRefreshVRAMData) {
			refresh16BitData();
			mRefreshVRAMData = ESX_FALSE;
		}

		for (I32 yOff = 0; yOff < height; yOff++) {
			for (I32 xOff = 0; xOff < width; xOff++) {
				U16 xImage = x + xOff;
				U16 yImage = (511 - y - yOff);

				xImage &= 1023;
				yImage &= 511;

				U64 index = (yImage * 1024) + xImage;

				pixels.emplace_back(mVRAM16.at(index));
			}
		}
	}

	void BatchRenderer::Reset()
	{
		std::fill(mTriVerticesBase.begin(), mTriVerticesBase.end(), PolygonVertex());
		mTriCurrentVertex = mTriVerticesBase.begin();

		std::fill(mLineStripIndicesBase.begin(), mLineStripIndicesBase.end(), 0);
		mLineStripCurrentIndex = mLineStripIndicesBase.begin();

		mFBO16->bind();
		glScissor(0, 0, 1024, 512);
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		mFBO16->unbind();

		mVRAM16.resize(1024 * 512);
		std::fill(mVRAM16.begin(), mVRAM16.end(), VRAMColor());

		mDrawTopLeft = glm::uvec2(0, 0);
		mDrawBottomRight = glm::uvec2(640, 240);
		mForceAlpha = ESX_FALSE;
		mCheckMask = ESX_FALSE;
		mVRAMWritePending = ESX_FALSE;

		glEnable(GL_SCISSOR_TEST);

		mRefreshVRAMData = ESX_TRUE;
	}

	void BatchRenderer::refresh16BitData()
	{
		FlushVRAMWrites();

		void* data = mVRAM16.data();
		mTexture16->bind();
		mTexture16->getPixels(&data);
		mTexture16->unbind();
	}

	void BatchRenderer::refresh24BitTexture()
	{

		/*mTexture24->bind();
		mPBO24Up->bind();
		void* dma24 = mPBO24Up->mapBuffer();
		if (dma24) {
			std::memcpy(dma24, mVRAM16.data(), 682 * 512 * sizeof(U8) * 3);
			mPBO24Up->unmapBuffer();
			mTexture24->copy(mPBO24Up);
		}
		mPBO24Up->unbind();
		mTexture24->unbind();
		glFlush();*/

		mTexture24->bind();
		mTexture24->setPixels(0, 0, 682, 512, mVRAM16.data());
		mTexture24->unbind();
	}

	void BatchRenderer::FlushVRAMWrites()
	{
		if (mVRAMWritePending) {
			mTexture16->bind();
			mPBO16Up->bind();
			VRAMColor* dma16 = reinterpret_cast<VRAMColor*>(mPBO16Up->mapBuffer());
			if (dma16) {
				std::memcpy(dma16, mVRAM16.data(), mVRAM16.size() * sizeof(VRAMColor));
				mPBO16Up->unmapBuffer();
				mTexture16->copy(mPBO16Up);
			}
			mPBO16Up->unbind();
			mTexture16->unbind();

			if (m24Bit) {
				refresh24BitTexture();
			}

			mVRAMWritePending = ESX_FALSE;
		}
	}

}