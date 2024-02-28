#pragma once

#include "Base/Assert.h"
#include "Base/Base.h"
#include "Core/IRenderer.h"

#include "UI/Graphics/Shader.h"
#include "UI/Graphics/VertexArray.h"

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

	public:
		static const size_t QUAD_VERTEX_SIZE = sizeof(PolygonVertex);
		static const size_t QUAD_SIZE = QUAD_VERTEX_SIZE * 4;
		static const size_t MAX_QUADS = 100;
		static const size_t QUAD_BUFFER_SIZE = QUAD_SIZE * MAX_QUADS;
		static const size_t QUAD_MAX_NUM_INDICES = MAX_QUADS * 6;


		static const size_t TRI_SIZE = QUAD_VERTEX_SIZE * 3;
		static const size_t MAX_TRIS = 100;
		static const size_t TRI_BUFFER_SIZE = TRI_SIZE * MAX_TRIS;
		static const size_t TRI_MAX_NUM_INDICES = MAX_TRIS * 3;
	private:
		std::shared_ptr<Shader> mShader;

		std::shared_ptr<VertexArray> mVAO;
		std::shared_ptr<VertexBuffer> mVBO;
		std::shared_ptr<IndexBuffer> mIBO;
		Vector<PolygonVertex> mVerticesBase;
		Vector<PolygonVertex>::iterator mCurrentVertex;
		uint32_t mNumIndices;

		std::shared_ptr<VertexArray> mTriVAO;
		std::shared_ptr<VertexBuffer> mTriVBO;
		std::shared_ptr<IndexBuffer> mTriIBO;
		Vector<PolygonVertex> mTriVerticesBase;
		Vector<PolygonVertex>::iterator mTriCurrentVertex;
		uint32_t mTriNumIndices;

		glm::ivec2 mDrawOffset = glm::ivec2(0,0);
		glm::uvec2 mDrawTopLeft = glm::uvec2(0,0);
		glm::uvec2 mDrawBottomRight = glm::uvec2(0,0);
	};

}