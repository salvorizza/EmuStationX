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
		void DrawPolygon(const Vector<PolygonVertex>& vertices) override;
		void DrawRectangle(const Vertex& topLeft, U16 width, U16 height, const Color& color) override;
		void VRAMWrite(U16 x, U16 y, U16 data) override;
		U16 VRAMRead(U16 x, U16 y) override;

		SharedPtr<FrameBuffer> getPreviousFrame() { return mFBO; }

	public:
		static const size_t QUAD_VERTEX_SIZE = sizeof(PolygonVertex);
		static const size_t TRI_SIZE = QUAD_VERTEX_SIZE * 3;
		static const size_t MAX_TRIS = 100;
		static const size_t TRI_BUFFER_SIZE = TRI_SIZE * MAX_TRIS;
		static const size_t TRI_MAX_NUM_INDICES = MAX_TRIS * 3;
	private:
		SharedPtr<Shader> mShader;
		SharedPtr<VertexArray> mTriVAO;
		SharedPtr<VertexBuffer> mTriVBO;
		SharedPtr<IndexBuffer> mTriIBO;
		Vector<PolygonVertex> mTriVerticesBase;
		Vector<PolygonVertex>::iterator mTriCurrentVertex;
		SharedPtr<FrameBuffer> mFBO;

		glm::ivec2 mDrawOffset = glm::ivec2(0,0);
		glm::uvec2 mDrawTopLeft = glm::uvec2(0,0);
		glm::uvec2 mDrawBottomRight = glm::uvec2(0,0);

		SharedPtr<PixelBuffer> mPBO4, mPBO8, mPBO16;
		SharedPtr<Texture2D> mTexture4, mTexture8, mTexture16;
		U8* mPixels4 = nullptr;
		U8* mPixels8 = nullptr;
		U16* mPixels16 = nullptr;

	};

}