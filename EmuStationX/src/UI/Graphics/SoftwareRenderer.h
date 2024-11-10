#pragma once

#include "Base/Assert.h"
#include "Base/Base.h"
#include "Core/IRenderer.h"

#include "UI/Graphics/Texture2D.h"
#include "UI/Graphics/FrameBuffer.h"

#include <glm/glm.hpp>
#include <glm/gtx/matrix_operation.hpp>
#include <glm/gtc/matrix_access.hpp>

namespace esx {

	class SoftwareRenderer : public IRenderer {
	public:
		SoftwareRenderer();
		~SoftwareRenderer() = default;

		void Begin() override;
		void Flush() override;
		void FlushVRAMWrites() override;

		void SetDrawOffset(I16 offsetX, I16 offsetY) override;
		void SetDrawTopLeft(U16 x, U16 y) override;
		void SetDrawBottomRight(U16 x, U16 y) override;
		void SetForceAlpha(BIT value) override;
		void SetCheckMask(BIT value) override;
		virtual void SetDisplayMode24(BIT value) override;
		void Clear(U16 x, U16 y, U16 w, U16 h, Color& color) override;
		void DrawPolygon(Array<PolygonVertex, 4>& vertices, U32 numVertices) override;
		void DrawLineStrip(Vector<PolygonVertex>& vertices) override;
		void VRAMWrite(U16 x, U16 y, U32 width, U32 height, const Vector<VRAMColor>& pixels) override;
		void VRAMRead(U16 x, U16 y, U32 width, U32 height, Vector<VRAMColor>& pixels) override;

		virtual void Reset() override;

		const SharedPtr<FrameBuffer>& getPreviousFrame() { return mFBO; }
	private:

		void quad(const PolygonVertex* vtx);
		void triangle(const PolygonVertex* vtx);

		void fragment(const glm::i32vec2& P, const glm::dvec3& bcCoords, const PolygonVertex* vtx, BIT& discard);
	private:
		SharedPtr<FrameBuffer> mFBO;
		SharedPtr<Texture2D> mTexture;

		glm::ivec2 mDrawOffset = glm::ivec2(0, 0);
		glm::uvec2 mDrawTopLeft = glm::uvec2(0, 0);
		glm::uvec2 mDrawBottomRight = glm::uvec2(0, 0);
		BIT mForceAlpha = ESX_FALSE;
		BIT mCheckMask = ESX_FALSE;
		BIT m24Bit = ESX_FALSE;
	};

}