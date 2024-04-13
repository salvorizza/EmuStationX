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
		void DrawPolygon(Vector<PolygonVertex>& vertices) override;
		void VRAMWrite(U16 x, U16 y, U16 data) override;
		U16 VRAMRead(U16 x, U16 y) override;

		SharedPtr<FrameBuffer> getPreviousFrame() { return mFBO; }

	public:
		static const size_t QUAD_VERTEX_SIZE = sizeof(PolygonVertex);
		static const size_t TRI_SIZE = QUAD_VERTEX_SIZE * 3;
		static const size_t MAX_TRIS = 100;
		static const size_t TRI_BUFFER_SIZE = TRI_SIZE * MAX_TRIS;
		static const size_t TRI_MAX_VERTICES = MAX_TRIS * 3;
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

		Vector<U8> mVRAM24;

	};

}