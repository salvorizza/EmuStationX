#pragma once

#include "Base/Base.h"

#include <glm/glm.hpp>

namespace esx {

	using Vertex = glm::i16vec2;
	using UV = glm::u16vec2;
	using Color = glm::u8vec3;

	struct PolygonVertex {
		Vertex vertex;
		UV uv;
		Color color;
		U8 textured = 0x00;
		UV clutUV;
		U8 bpp = 0x00;
		U8 semiTransparency = 0x00;
		U8 dither = 0x00;
		U8 rawTexture = 0x00;
	};


	union VRAMColor {
		U16 data;
	};


	constexpr size_t VRAM_WIDTH = 1024;
	constexpr size_t VRAM_HEIGHT = 512;

	constexpr size_t p = sizeof(VRAMColor);

	class IRenderer {
	public:
		virtual ~IRenderer() = default;

		virtual void Flush() = 0;
		virtual void FlushVRAMWrites() = 0;
		virtual void Begin() = 0;
		virtual void SetDrawOffset(I16 offsetX, I16 offsetY) = 0;
		virtual void SetDrawTopLeft(U16 x, U16 y) = 0;
		virtual void SetDrawBottomRight(U16 x, U16 y) = 0;
		virtual void SetForceAlpha(BIT value) = 0;
		virtual void SetCheckMask(BIT value) = 0;
		virtual void SetDisplayMode24(BIT value) = 0;
		virtual void SetTextureWindow(U32 maskX, U32 maskY, U32 offsetMaskX, U32 offsetMaskY) = 0;
		virtual void Clear(U16 x, U16 y, U16 w, U16 h, const Color& color) = 0;
		virtual void DrawPolygon(Array<PolygonVertex, 4>& vertices, U32 numVertices) = 0;
		virtual void DrawLineStrip(Vector<PolygonVertex>& vertices) = 0;

		virtual void Reset() = 0;

		virtual void VRAMWrite(U16 x, U16 y, U32 width, U32 height, const Vector<VRAMColor>& pixels) = 0;
		virtual void VRAMRead(U16 x, U16 y, U32 width, U32 height, Vector<VRAMColor>& pixels) = 0;

		static VRAMColor fromU16(U16 value) {
			VRAMColor color;
			
			/*U8 r = (value >> 0) & 0x1F;
			U8 g = (value >> 5) & 0x1F;
			U8 b = (value >> 10) & 0x1F;
			U8 a = (value >> 15) & 0x1;

			color.data = (r << 11) | (g << 6) | (b << 1) | a;*/
			color.data = value;

			return color;

		}

		static U16 toU16(const VRAMColor& color) {
			/*U8 r = (color.data >> 11) & 0x1F;
			U8 g = (color.data >> 6) & 0x1F;
			U8 b = (color.data >> 1) & 0x1F;
			U8 a = (color.data >> 0) & 0x1;

			return (a << 15) | (b << 10) | (g << 5) | r;*/
			return color.data;
		}


		void SaveToFile(const std::filesystem::path& outPath) {
			std::ofstream file(outPath, std::ios::binary);
			if (!file) {
				return;
			}

			file << "P6\n";
			file << VRAM_WIDTH << " " << VRAM_HEIGHT << "\n";
			file << "255\n";

			// Genera l'immagine pixel per pixel
			for (I32 y = 0; y < VRAM_HEIGHT; y++) {
				for (I32 x = 0; x < VRAM_WIDTH; x++) {
					glm::vec4 color = colorConvert(mVRAM[(VRAM_HEIGHT - 1 - y) * VRAM_WIDTH + x]);


					file.put(static_cast<char>(color.r));
					file.put(static_cast<char>(color.g));
					file.put(static_cast<char>(color.b));
				}
			}

			file.close();
		}

	protected:
		inline glm::u8vec4 colorConvert(const VRAMColor& color) const {
			glm::u8vec4 result = glm::u8vec4(0);

			result.r = static_cast<U8>(((color.data >> 0) & 0x1F) / 31.0f * 255.0f);
			result.g = static_cast<U8>(((color.data >> 5) & 0x1F) / 31.0f * 255.0f);
			result.b = static_cast<U8>(((color.data >> 10) & 0x1F) / 31.0f * 255.0f);
			result.a = (color.data >> 15) == 1 ? 255 : 0;

			return result;
		}
		inline VRAMColor colorConvert(const Color& color, BIT alpha = ESX_FALSE) {
			return VRAMColor(((alpha ? 1 : 0) << 15) | ((static_cast<U16>(color.b) >> 3) << 10) | ((static_cast<U16>(color.g) >> 3) << 5) | ((static_cast<U16>(color.r) >> 3) << 0));
		}
		inline void setAlpha(VRAMColor& color, BIT alpha) {
			color = VRAMColor((color.data & 0x7FFF) | (alpha ? 0x8000 : 0x0000));
		}
	protected:
		Vector<VRAMColor> mVRAM;
	};
}