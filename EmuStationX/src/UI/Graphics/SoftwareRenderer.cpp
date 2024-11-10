#include "SoftwareRenderer.h"

namespace esx {

	constexpr glm::mat4x4 dither = glm::mat4x4(
		glm::vec4(-4, +0, -3, +1),
		glm::vec4(+2, -2, +3, -1),
		glm::vec4(-3, +1, -4, +0),
		glm::vec4(+3, -1, +2, -2)
	);

	glm::dvec3 barycentric(const glm::dvec2& a, const glm::dvec2& b, const glm::dvec2& c, const glm::dvec2& p) {
		glm::dvec2 v0 = b - a, v1 = c - a, v2 = p - a;
		F64 den = v0.x * v1.y - v1.x * v0.y;
		F64 v = (v2.x * v1.y - v1.x * v2.y) / den;
		F64 w = (v0.x * v2.y - v2.x * v0.y) / den;
		F64 u = 1.0f - v - w;

		const F64 epsilon = 1e-10;
		if (std::abs(u) < epsilon) u = 0.0f;
		if (std::abs(v) < epsilon) v = 0.0f;
		if (std::abs(w) < epsilon) w = 0.0f;

		return glm::dvec3(u, v, w);
	}	

	void apply_dither(glm::vec3& color, const glm::vec2& P) {
		I32 x = I32(P.x) % 4;
		I32 y = I32(P.y) % 4;
		color += dither[y][x];		
		color = glm::clamp(color, glm::vec3(0), glm::vec3(255));
	}

	U8 texel_8bit(const glm::u32vec2& coords, VRAMColor* vram) {
		glm::u32vec2 vram_coords = glm::u32vec2(coords.x >> 1, 511 - coords.y);
		vram_coords %= glm::u32vec2(VRAM_WIDTH, VRAM_HEIGHT);
		U16 data = vram[vram_coords.y * VRAM_WIDTH + vram_coords.x].data;
		U32 shift = (coords.x & 1) << 3;
		U8 texel = (data >> shift) & 0xFF;
		return texel;
	}

	U8 texel_4bit(const glm::u32vec2& coords, VRAMColor* vram) {
		glm::u32vec2 vram_coords = glm::u32vec2(coords.x >> 2, 511 - coords.y);
		vram_coords %= glm::u32vec2(VRAM_WIDTH, VRAM_HEIGHT);
		U16 data = vram[vram_coords.y * VRAM_WIDTH + vram_coords.x].data;
		U32 shift = (coords.x & 3) << 2;
		U8 texel = (data >> shift) & 0xF;
		return texel;
	}

	glm::vec3 blend_colors(const glm::vec3& background, const glm::vec3& foreground, U8 blendFunc) {
		glm::vec3 result = glm::vec3(0);

		switch (blendFunc) {
			case 0: {
				result = 0.5f * background + 0.5f * foreground;
				break;
			}

			case 1: {
				result = 1.0f * background + 1.0f * foreground;
				break;
			}

			case 2: {
				result = 1.0f * background - 1.0f * foreground;
				break;
			}

			case 3: {
				result = 1.0f * background + 0.25f * foreground;
				break;
			}

			default: {
				result = foreground;
				break;
			}
		}

		result = glm::clamp(result, glm::vec3(0), glm::vec3(255));

		return result;
	}

	SoftwareRenderer::SoftwareRenderer()
	{
		mFBO = MakeShared<FrameBuffer>(1024, 512);
		mTexture = MakeShared<Texture2D>(0);
		mTexture->setData(nullptr, 1024, 512, InternalFormat::RGB5_A1, DataType::UnsignedShort1_555, DataFormat::RGBA);
		mFBO->setColorAttachment(mTexture);
		mFBO->init();

		mVRAM.resize(1024 * 512);
	}

	void SoftwareRenderer::Begin()
	{
	}

	void SoftwareRenderer::Flush()
	{
		mTexture->bind();
		mTexture->setPixels(0, 0, VRAM_WIDTH, VRAM_HEIGHT, mVRAM.data());
		mTexture->unbind();
	}

	void SoftwareRenderer::FlushVRAMWrites()
	{
	}

	void SoftwareRenderer::SetDrawOffset(I16 offsetX, I16 offsetY)
	{
		mDrawOffset.x = offsetX;
		mDrawOffset.y = offsetY;
	}

	void SoftwareRenderer::SetDrawTopLeft(U16 x, U16 y)
	{
		mDrawTopLeft.x = x;
		mDrawTopLeft.y = y;
	}

	void SoftwareRenderer::SetDrawBottomRight(U16 x, U16 y)
	{
		mDrawBottomRight.x = x;
		mDrawBottomRight.y = y;
	}

	void SoftwareRenderer::SetForceAlpha(BIT value)
	{
		mForceAlpha = value;
	}

	void SoftwareRenderer::SetCheckMask(BIT value)
	{
		mCheckMask = value;
	}

	void SoftwareRenderer::SetDisplayMode24(BIT value)
	{
		m24Bit = value;
	}

	void SoftwareRenderer::Clear(U16 x, U16 y, U16 w, U16 h, Color& color)
	{
		VRAMColor color16 = colorConvert(Color(color.r, color.g, color.b));
		for (U16 yIndex = 0; yIndex < h; yIndex++) {
			for (U16 xIndex = 0; xIndex < w; xIndex++) {
				U16 xPos = (x + xIndex) % VRAM_WIDTH;
				U16 yPos = (y + yIndex) % VRAM_HEIGHT;
				yPos = 511 - yPos;

				U32 index = yPos * VRAM_WIDTH + xPos;
				mVRAM[index] = color16;
			}
		}
	}

	void SoftwareRenderer::DrawPolygon(Array<PolygonVertex, 4>& vertices, U32 numVertices)
	{
		for (PolygonVertex& vertex : vertices) {
			vertex.vertex.x += mDrawOffset.x;
			vertex.vertex.y += mDrawOffset.y;
		}

		triangle(&vertices[0]);
		if (numVertices == 4) {
			triangle(&vertices[1]);
		}
	}

	void SoftwareRenderer::DrawLineStrip(Vector<PolygonVertex>& vertices)
	{
	}

	void SoftwareRenderer::VRAMWrite(U16 x, U16 y, U32 width, U32 height, const Vector<VRAMColor>& pixels)
	{
		for (U16 yIndex = 0; yIndex < height; yIndex++) {
			for (U16 xIndex = 0; xIndex < width; xIndex++) {
				U16 xPos = (x + xIndex) % VRAM_WIDTH;
				U16 yPos = (y + yIndex) % VRAM_HEIGHT;
				yPos = 511 - yPos;

				U32 indexVRAM = yPos * VRAM_WIDTH + xPos;
				U32 indexPixels = yIndex * width + xIndex;

				VRAMColor color = pixels.at(indexPixels);

				if (mCheckMask && (mVRAM[indexVRAM].data & 0x8000) == 0x8000) continue;
				if (mForceAlpha) color.data |= 0x8000;

				mVRAM[indexVRAM] = color;
			}
		}
	}

	void SoftwareRenderer::VRAMRead(U16 x, U16 y, U32 width, U32 height, Vector<VRAMColor>& pixels)
	{
		for (I32 yIndex = 0; yIndex < height; yIndex++) {
			for (I32 xIndex = 0; xIndex < width; xIndex++) {
				U16 xPos = (x + xIndex) % VRAM_WIDTH;
				U16 yPos = (y + yIndex) % VRAM_HEIGHT;
				yPos = 511 - yPos;

				U64 index = yPos * VRAM_WIDTH + xPos;

				pixels.emplace_back(mVRAM.at(index));
			}
		}
	}

	void SoftwareRenderer::Reset()
	{
		mVRAM.resize(1024 * 512);
		std::fill(mVRAM.begin(), mVRAM.end(), VRAMColor());

		mDrawTopLeft = glm::uvec2(0, 0);
		mDrawBottomRight = glm::uvec2(640, 240);
		mForceAlpha = ESX_FALSE;
		mCheckMask = ESX_FALSE;
	}

	void SoftwareRenderer::triangle(const PolygonVertex* vtx) {
		glm::i32vec2 bboxmin(1024, 512);
		glm::i32vec2 bboxmax(-1024, -512);
		glm::i32vec2 clampMin(mDrawTopLeft.x, mDrawTopLeft.y);
		glm::i32vec2 clampMax(mDrawBottomRight.x, mDrawBottomRight.y);
		for (I32 i = 0; i < 3; i++) {
			bboxmin.x = std::max<int>(clampMin.x, std::min<int>(bboxmin.x, vtx[i].vertex.x));
			bboxmin.y = std::max<int>(clampMin.y, std::min<int>(bboxmin.y, vtx[i].vertex.y));

			bboxmax.x = std::min<int>(clampMax.x, std::max<int>(bboxmax.x, vtx[i].vertex.x));
			bboxmax.y = std::min<int>(clampMax.y, std::max<int>(bboxmax.y, vtx[i].vertex.y));
		}

		#pragma omp parallel for
		for (I32 y = bboxmin.y; y <= bboxmax.y; y++) {
			glm::i32vec2 P;
			P.y = y;
			for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++) {
				glm::dvec3 bc_screen = barycentric(vtx[0].vertex, vtx[1].vertex, vtx[2].vertex, P);
				if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) continue;
				BIT discarded = ESX_FALSE;
				fragment(P, bc_screen, vtx, discarded);
			}
		}
	}

	void SoftwareRenderer::fragment(const glm::i32vec2& P, const glm::dvec3& bcCoords, const PolygonVertex* vtx, BIT& discard)
	{
		U32 index = ((VRAM_HEIGHT - 1 - int(P.y)) % VRAM_HEIGHT) * VRAM_WIDTH + (int(P.x) % VRAM_WIDTH);

		glm::vec4 backColor = colorConvert(mVRAM[index]);
		if (mCheckMask == 1 && backColor.a == 255) {
			discard = ESX_TRUE;
			return;
		}

		glm::vec2 uv = (bcCoords.x * glm::dvec2(vtx[0].uv)) + (bcCoords.y * glm::dvec2(vtx[1].uv)) + (bcCoords.z * glm::dvec2(vtx[2].uv));
		glm::vec3 frontColor = (bcCoords.x * glm::dvec3(vtx[0].color)) + (bcCoords.y * glm::dvec3(vtx[1].color)) + (bcCoords.z * glm::dvec3(vtx[2].color));

		uv.s = round(uv.s);
		uv.t = round(uv.t);

		VRAMColor color;
		if (vtx[0].textured == 1) {
			glm::u32vec2 uvColor = glm::u32vec2(0, 0);


			switch (vtx[0].bpp) {
				case 4: {
					uvColor = glm::u32vec2(vtx[0].clutUV.s + texel_4bit(glm::i32vec2(uv), mVRAM.data()), 511 - vtx[0].clutUV.t);
					break;
				}

				case 8: {
					uvColor = glm::u32vec2(vtx[0].clutUV.s + texel_8bit(glm::i32vec2(uv), mVRAM.data()), 511 - vtx[0].clutUV.t);
					break;
				}

				case 16: {
					uvColor = glm::u32vec2(uv.x, 511 - uv.y);
					break;
				}
			}

			uvColor %= glm::u32vec2(VRAM_WIDTH, VRAM_HEIGHT);
			U32 uvIndex = uvColor.y * VRAM_WIDTH + uvColor.x;
			glm::vec4 texelColor = colorConvert(mVRAM[uvIndex]);

			if (texelColor == glm::vec4(0, 0, 0, 0)) {
				discard = ESX_TRUE;
				return;
			}

			if (vtx[0].rawTexture == 0) {
				texelColor = texelColor * (glm::vec4(frontColor, 128.0f) / 128.0f);
				texelColor = glm::clamp(texelColor, glm::vec4(0), glm::vec4(255));
			}

			if (texelColor.a > 0) {
				frontColor = blend_colors(backColor, texelColor, vtx[0].semiTransparency);
			}
			else {
				frontColor = texelColor;
			}

			if (vtx[0].rawTexture == 0u && vtx[0].dither == 1u) {
				apply_dither(frontColor, P);
			}
		}
		else {
			if (vtx[0].dither == 1u) {
				apply_dither(frontColor, P);
			}

			frontColor = blend_colors(backColor, frontColor, vtx[0].semiTransparency);
		}
		color = colorConvert(Color(frontColor.r, frontColor.g, frontColor.b));

		if (mForceAlpha) {
			setAlpha(color, ESX_TRUE);
		}

		mVRAM[index] = color;
	}
}