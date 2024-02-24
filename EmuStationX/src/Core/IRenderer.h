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
		U8 u;
		U8 v;

		UV()
			:	u(0),
				v(0)
		{}

		UV(U8 _u, U8 _v)
			:	u(_u),
				v(_v)
		{}
	};

	struct Color {
		U8 r;
		U8 g;
		U8 b;
		U8 a;

		Color()
			:	r(0),
				g(0),
				b(0),
				a(0)
		{}

		Color(U8 _r, U8 _g, U8 _b)
			:	r(_r),
				g(_g),
				b(_b),
				a(0)
		{}
	};

	struct PolygonVertex {
		Vertex vertex;
		UV uv;
		Color color;
	};

	class IRenderer {
	public:
		virtual ~IRenderer() = default;

		virtual void Flush() = 0;
		virtual void Begin() = 0;
		virtual void DrawPolygon(const Vector<PolygonVertex>& vertices) = 0;
		virtual void DrawRectangle(const Vertex& topLeft, U16 width, U16 height, const Color& color) = 0;

	};
}