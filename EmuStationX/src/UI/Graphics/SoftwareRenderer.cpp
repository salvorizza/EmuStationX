#include "SoftwareRenderer.h"

#include <execution>

namespace esx {

	constexpr glm::mat4x4 dither = glm::mat4x4(
		glm::vec4(-4, +0, -3, +1),
		glm::vec4(+2, -2, +3, -1),
		glm::vec4(-3, +1, -4, +0),
		glm::vec4(+3, -1, +2, -2)
	);

	I32 orient2d(const glm::i32vec2& a, const glm::i32vec2& b, const glm::i32vec2& c) {
		return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
	}

	glm::i32vec3 barycentric(const glm::i32vec2& a, const glm::i32vec2& b, const glm::i32vec2& c, const glm::i32vec2& p) {
		I32 u = orient2d(b, c, p);
		I32 v = orient2d(c, a, p);
		I32 w = orient2d(a, b, p);

		return glm::i32vec3(u, v, w);
	}	

	glm::i32vec3 apply_dither(const glm::i32vec3& color, const glm::i32vec2& P) {
		I32 x = I32(P.x) % 4;
		I32 y = I32(P.y) % 4;
		return glm::clamp(color + glm::i32vec3(dither[y][x]), glm::i32vec3(0), glm::i32vec3(255));
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

	template<I32 N>
	Pair<glm::i32vec2, glm::i32vec2> calculate_bounding(const PolygonVertex* vtx) {
		Pair<glm::i32vec2, glm::i32vec2> result = std::make_pair(glm::i32vec2(vtx[0].vertex.x % VRAM_WIDTH, vtx[0].vertex.y % VRAM_HEIGHT), glm::i32vec2(vtx[0].vertex.x % VRAM_WIDTH, vtx[0].vertex.y % VRAM_HEIGHT));

		for (I32 i = 1; i < N; i++) {
			result.first.x = std::min<I32>(result.first.x, vtx[i].vertex.x % VRAM_WIDTH);
			result.first.y = std::min<I32>(result.first.y, vtx[i].vertex.y % VRAM_HEIGHT);

			result.second.x = std::max<I32>(result.second.x, vtx[i].vertex.x % VRAM_WIDTH);
			result.second.y = std::max<I32>(result.second.y, vtx[i].vertex.y % VRAM_HEIGHT);
		}

		return result;
	}

	glm::u8vec3 blend_colors(const glm::i32vec3& background, const glm::i32vec3& foreground, U8 blendFunc) {
		glm::i32vec3 result = glm::i32vec3(0);

		switch (blendFunc) {
			case 0: {
				result = (background + foreground) / 2;
				break;
			}

			case 1: {
				result = background + foreground;
				break;
			}

			case 2: {
				result = background - foreground;
				break;
			}

			case 3: {
				result = (4 * background + foreground) / 4;
				break;
			}

			default: {
				result = foreground;
				break;
			}
		}

		result = glm::clamp(result, glm::i32vec3(0), glm::i32vec3(255));

		return result;
	}

	void ensureCounterClockwise(PolygonVertex* vtx) {
		int det = (vtx[1].vertex.x - vtx[0].vertex.x) * (vtx[2].vertex.y - vtx[0].vertex.y) -
			(vtx[1].vertex.y - vtx[0].vertex.y) * (vtx[2].vertex.x - vtx[0].vertex.x);

		if (det < 0) {  // Se l'ordine è orario
			std::swap(vtx[1], vtx[2]);
		}
	}

	SoftwareRenderer::SoftwareRenderer()
	{
		mFBO = MakeShared<FrameBuffer>(1024, 512);
		mTexture = MakeShared<Texture2D>(0);
		mTexture->setData(nullptr, 1024, 512, InternalFormat::RGB5_A1, DataType::UnsignedShort1_555, DataFormat::RGBA);
		mFBO->setColorAttachment(mTexture);
		mFBO->init();

		mVRAM.resize(1024 * 512);
		mVRAMFront.resize(1024 * 512);
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

	void SoftwareRenderer::SetTextureWindow(U32 maskX, U32 maskY, U32 offsetMaskX, U32 offsetMaskY)
	{
		mTextureWindow = glm::ivec4(~maskX, ~maskY, offsetMaskX & maskX, offsetMaskY & maskY);
	}

	void SoftwareRenderer::Clear(U16 x, U16 y, U16 w, U16 h, const Color& color)
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

		auto boundingToCopy = numVertices == 4 ? calculate_bounding<4>(&vertices[0]) : calculate_bounding<3>(&vertices[0]);
		for (I32 y = boundingToCopy.first.y; y <= boundingToCopy.second.y; y++) {
			for (I32 x = boundingToCopy.first.x; x <= boundingToCopy.second.x; x++) {
				mVRAMFront[(511 - y) * VRAM_WIDTH +x] = mVRAM[(511 - y) * VRAM_WIDTH + x];
			}
		}

		triangle(&vertices[0]);
		if (numVertices == 4) {
			triangle(&vertices[1]);
		}

		for (I32 y = boundingToCopy.first.y; y <= boundingToCopy.second.y; y++) {
			for (I32 x = boundingToCopy.first.x; x <= boundingToCopy.second.x; x++) {
				mVRAM[(511 - y) * VRAM_WIDTH + x] = mVRAMFront[(511 - y) * VRAM_WIDTH + x];
			}
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

	void SoftwareRenderer::triangle(PolygonVertex* vtx) {
		ensureCounterClockwise(vtx);

		auto bounding = calculate_bounding<3>(vtx);

		// Compute triangle bounding box
		I32 minX = bounding.first.x;
		I32 minY = bounding.first.y;
		I32 maxX = bounding.second.x;
		I32 maxY = bounding.second.y;

		// Clip against screen bounds
		minX = std::max<I32>(minX, mDrawTopLeft.x);
		minY = std::max<I32>(minY, mDrawTopLeft.y);
		maxX = std::min<I32>(maxX, mDrawBottomRight.x);
		maxY = std::min<I32>(maxY, mDrawBottomRight.y);

		glm::i32vec3 A = glm::i32vec3(
			vtx[1].vertex.y - vtx[2].vertex.y,
			vtx[2].vertex.y - vtx[0].vertex.y,
			vtx[0].vertex.y - vtx[1].vertex.y
		);

		glm::i32vec3 B = glm::i32vec3(
			vtx[2].vertex.x - vtx[1].vertex.x,
			vtx[0].vertex.x - vtx[2].vertex.x,
			vtx[1].vertex.x - vtx[0].vertex.x
		);

		glm::i32vec2 minP(minX, minY);
		glm::i32vec3 minBarycentric = barycentric(vtx[0].vertex, vtx[1].vertex, vtx[2].vertex, minP);

		if ((maxY - minY + 1) >= 0) {
			std::vector<I32> yRange(maxY - minY + 1);
			std::iota(yRange.begin(), yRange.end(), minY);

			std::for_each(
				#if defined(ESX_RELEASE)
					std::execution::par
				#else
					std::execution::seq
				#endif
				,yRange.begin(), yRange.end(), [&](I32 y) {
				glm::i32vec3 w_row = minBarycentric + (y - minY) * B;
				glm::i32vec3 w = w_row;

				for (I32 x = minX; x <= maxX; x++) {
					if ((w.x | w.y | w.z) >= 0) {
						glm::i32vec2 P(x, y);
						BIT discarded = ESX_FALSE;
						fragment(P, w, vtx, 3, discarded);
					}

					w += A;
				}
				});
		}
	}

	void SoftwareRenderer::fragment(const glm::i32vec2& P, const glm::i32vec3& bcCoords, const PolygonVertex* vtx, I32 numVertices, BIT& discard)
	{
		U32 index = ((VRAM_HEIGHT - 1 - int(P.y)) % VRAM_HEIGHT) * VRAM_WIDTH + (int(P.x) % VRAM_WIDTH);

		glm::u8vec4 backColor = colorConvert(mVRAM[index]);
		if (mCheckMask == 1 && backColor.a == 255) {
			discard = ESX_TRUE;
			return;
		}

		I32 weightSum = bcCoords.x + bcCoords.y + bcCoords.z;
		weightSum = std::max(weightSum, 1);
		glm::i32vec2 uv = (bcCoords.x * glm::i32vec2(vtx[0].uv) + bcCoords.y * glm::i32vec2(vtx[1].uv) + bcCoords.z * glm::i32vec2(vtx[2].uv)) / weightSum;
		glm::i32vec3 frontColor = (bcCoords.x * glm::i32vec3(vtx[0].color) + bcCoords.y * glm::i32vec3(vtx[1].color) + bcCoords.z * glm::i32vec3(vtx[2].color)) / weightSum;

		uv = (uv & glm::i32vec2(mTextureWindow.x, mTextureWindow.y)) | glm::i32vec2(mTextureWindow.z, mTextureWindow.w);

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
			glm::i32vec4 texelColor = colorConvert(mVRAM[uvIndex]);

			if (texelColor == glm::i32vec4(0, 0, 0, 0)) {
				discard = ESX_TRUE;
				return;
			}

			if (vtx[0].rawTexture == 0) {
				glm::i32vec4 modulatedColor = (glm::i32vec4(texelColor) * glm::i32vec4(frontColor, 128)) / 128;
				texelColor = glm::clamp(modulatedColor, glm::i32vec4(0), glm::i32vec4(255));

				if (vtx[0].dither == 1u) {
					texelColor = glm::i32vec4(apply_dither(texelColor, P), texelColor.a);
				}
			}

			frontColor = texelColor;

			if (texelColor.a > 0 && vtx[0].semiTransparency != 255) {
				frontColor = blend_colors(backColor, frontColor, vtx[0].semiTransparency);
			}
		}
		else {
			if (vtx[0].dither == 1u) {
				frontColor = apply_dither(frontColor, P);
			}

			if (vtx[0].semiTransparency != 255) {
				frontColor = blend_colors(backColor, frontColor, vtx[0].semiTransparency);
			}
		}
		color = colorConvert(Color(frontColor.r, frontColor.g, frontColor.b));

		if (mForceAlpha) {
			setAlpha(color, ESX_TRUE);
		}

		mVRAMFront[index] = color;
	}
}