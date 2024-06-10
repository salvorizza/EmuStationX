#pragma once

#include "Base/Base.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

namespace esx {
	#define BYTE(x) x
	#define KIBI(x) x * BYTE(1024)
	#define MIBI(x) x * KIBI(1024)

	struct DataBuffer {
		uint8_t* Data;
		size_t Size;

		DataBuffer()
			:	Data(NULL),
				Size(0)
		{}

		DataBuffer(uint8_t* data, size_t size)
			:	Data(data),
				Size(size)
		{}
	};

	struct ICOHeader {
		U16 Reserved;
		U16 ImageType;
		U16 ImageCount;
	};

	struct ICOImageEntry {
		U8 Width;
		U8 Height;
		U8 NumPalettes;
		U8 Reserved;
		U16 ColorPlanes;
		U16 BPP;
		U32 ImageSize;
		U32 ImageOffset;
	};

	struct ICOImage {
		ICOImageEntry ICOEntry;
		Vector<U8> Data;
	};

	errno_t ReadFile(const char* fileName, DataBuffer& outBuffer);
	errno_t WriteFile(const char* fileName, DataBuffer buffer);
	void DeleteBuffer(DataBuffer& buffer);

	Vector<ICOImage> ReadICO(const String& path);
}