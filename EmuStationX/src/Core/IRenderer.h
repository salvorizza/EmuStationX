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
	};

	class IRenderer {
	public:
		virtual ~IRenderer() = default;

		virtual void Flush() = 0;
		virtual void Begin() = 0;
		virtual void SetDrawOffset(I16 offsetX, I16 offsetY) = 0;
		virtual void SetDrawTopLeft(U16 x, U16 y) = 0;
		virtual void SetDrawBottomRight(U16 x, U16 y) = 0;
		virtual void DrawPolygon(Vector<PolygonVertex>& vertices) = 0;

		virtual void VRAMWrite(U16 x, U16 y, U16 data) = 0;
		virtual U16 VRAMRead(U16 x, U16 y) = 0;
	};
}