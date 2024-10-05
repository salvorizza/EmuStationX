#pragma once

#include "Base/Base.h"

namespace esx {
	struct Vertex {
		I16 x;
		I16 y;

		Vertex()
			:	x(0),
				y(0)
		{}

		Vertex(I16 _x,I16 _y)
			:	x(_x),
				y(_y)
		{}	
	};

	struct UV {
		U16 u;
		U16 v;

		UV()
			:	u(0),
				v(0)
		{}

		UV(U16 _u, U16 _v)
			:	u(_u),
				v(_v)
		{}
	};

	struct Color {
		U8 r;
		U8 g;
		U8 b;

		Color()
			:	r(0),
				g(0),
				b(0)
		{}

		Color(U8 _r, U8 _g, U8 _b)
			:	r(_r),
				g(_g),
				b(_b)
		{}
	};

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
		virtual void Clear(U16 x, U16 y, U16 w, U16 h, Color& color) = 0;
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
	};
}