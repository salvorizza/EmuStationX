#pragma once

#include "Base/Assert.h"
#include "Base/Base.h"
#include "Core/IRenderer.h"

#include "UI/Graphics/Shader.h"
#include "UI/Graphics/VertexArray.h"
#include "UI/Graphics/PixelBuffer.h"
#include "UI/Graphics/Texture2D.h"
#include "UI/Graphics/FrameBuffer.h"

#include <glm/glm.hpp>

namespace esx {

	class BatchRenderer : public IRenderer {
	public:
		BatchRenderer();
		~BatchRenderer() = default;

		void Begin();
		void end();
		void Flush() override;

		void SetDrawOffset(I16 offsetX, I16 offsetY) override;
		void SetDrawTopLeft(U16 x, U16 y) override;
		void SetDrawBottomRight(U16 x, U16 y) override;
		void SetForceAlpha(BIT value) override;
		void SetCheckMask(BIT value) override;
		virtual void SetDisplayMode24(BIT value) override;
		void Clear(U16 x, U16 y, U16 w, U16 h, Color& color) override;
		void DrawPolygon(Vector<PolygonVertex>& vertices) override;
		void DrawLineStrip(Vector<PolygonVertex>& vertices) override;
		void VRAMWrite(U16 x, U16 y, U32 width, U32 height, const Vector<VRAMColor>& pixels) override;
		void VRAMRead(U16 x, U16 y, U32 width, U32 height, Vector<VRAMColor>& pixels) override;

		const SharedPtr<FrameBuffer>& getPreviousFrame() { return m24Bit ? mFBO24 : mFBO16; }

		virtual void Reset() override;

	private:
		void refresh16BitData();
		void refresh24BitTexture();

	public:
		static const size_t QUAD_VERTEX_SIZE = sizeof(PolygonVertex);
		static const size_t TRI_SIZE = QUAD_VERTEX_SIZE * 3;
		static const size_t MAX_TRIS = 10000;
		static const size_t TRI_BUFFER_SIZE = TRI_SIZE * MAX_TRIS;
		static const size_t TRI_MAX_VERTICES = MAX_TRIS * 3;

		static const size_t LINE_STRIP_VERTEX_SIZE = sizeof(PolygonVertex);
		static const size_t MAX_LINE_STRIP_VERTICES = 10000;
		static const size_t LINE_STRIP_BUFFER_SIZE = LINE_STRIP_VERTEX_SIZE * MAX_LINE_STRIP_VERTICES;
	private:
		SharedPtr<Shader> mShader;


		SharedPtr<VertexArray> mTriVAO;
		SharedPtr<VertexBuffer> mTriVBO;
		Vector<PolygonVertex> mTriVerticesBase;
		Vector<PolygonVertex>::iterator mTriCurrentVertex;

		SharedPtr<VertexArray> mLineStripVAO;
		SharedPtr<VertexBuffer> mLineStripVBO;
		SharedPtr<IndexBuffer> mLineStripIBO;
		Vector<PolygonVertex> mLineStripVerticesBase;
		Vector<PolygonVertex>::iterator mLineStripCurrentVertex;
		Vector<U32> mLineStripIndicesBase;
		Vector<U32>::iterator mLineStripCurrentIndex;


		SharedPtr<FrameBuffer> mFBO16;
		SharedPtr<Texture2D> mTexture16;

		SharedPtr<FrameBuffer> mFBO24;
		SharedPtr<Texture2D> mTexture24;

		glm::ivec2 mDrawOffset = glm::ivec2(0,0);
		glm::uvec2 mDrawTopLeft = glm::uvec2(0,0);
		glm::uvec2 mDrawBottomRight = glm::uvec2(0,0);
		BIT mForceAlpha = ESX_FALSE;
		BIT mCheckMask = ESX_FALSE;

		Vector<VRAMColor> mVRAM16;
		BIT mRefreshVRAMData = ESX_FALSE;

		BIT m24Bit = ESX_FALSE;
	};

}