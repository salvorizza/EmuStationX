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
		struct QuadVertex {
			glm::vec2 Position;
			glm::vec2 UV;
			uint32_t Color;
		};

	public:
		BatchRenderer();
		~BatchRenderer();

		void begin(const glm::mat4& projMatrix, float borderWidth, float aspectRatio);
		void end();
		void flush();

		void drawQuad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);

		void DrawPolygon(const Vector<PolygonVertex>& vertices) override;
		void DrawRectangle(const Vertex& topLeft, U16 width, U16 height, const Color& color) override;

	public:
		static const size_t QUAD_VERTEX_SIZE = sizeof(QuadVertex);
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
		QuadVertex* mVerticesBase;
		QuadVertex* mCurrentVertex;
		uint32_t mNumIndices;

		std::shared_ptr<VertexArray> mTriVAO;
		std::shared_ptr<VertexBuffer> mTriVBO;
		std::shared_ptr<IndexBuffer> mTriIBO;
		QuadVertex* mTriVerticesBase;
		QuadVertex* mTriCurrentVertex;
		uint32_t mTriNumIndices;

		glm::mat4 mProjectionMatrix;
		float mBorderWidth, mAspectRatio;
	};

}