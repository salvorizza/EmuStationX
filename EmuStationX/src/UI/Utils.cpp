#include "Utils.h"

namespace esx {

	errno_t ReadFile(const char* fileName, DataBuffer& outBuffer)
	{
		FILE* f = NULL;
		errno_t err;

		err = fopen_s(&f, fileName, "rb");
		if (err == 0) {
			fseek(f, 0, SEEK_END);
			outBuffer.Size = ftell(f);
			fseek(f, 0, SEEK_SET);

			outBuffer.Data = (uint8_t*)malloc(sizeof(uint8_t) * outBuffer.Size);
			if (outBuffer.Data) {
				fread_s(outBuffer.Data, outBuffer.Size, outBuffer.Size, 1, f);
			}

			fclose(f);
		}

		return err;
	}

	errno_t WriteFile(const char* fileName, DataBuffer buffer)
	{
		FILE* f = NULL;
		errno_t err;

		err = fopen_s(&f, fileName, "wb");
		if (err == 0) {
			fwrite(buffer.Data, buffer.Size, 1, f);
			fclose(f);
		}

		return err;
	}

	void DeleteBuffer(DataBuffer& buffer)
	{
		free(buffer.Data);
		buffer.Size = 0;
		buffer.Data = NULL;
	}
	Vector<ICOImage> ReadICO(const String& path)
	{
		Vector<ICOImage> result;

		FILE* f = NULL;
		errno_t err;

		err = fopen_s(&f, path.c_str(), "rb");
		if (err == 0) {
			ICOHeader header = {};
			fread_s(&header, sizeof(ICOHeader), sizeof(ICOHeader), 1, f);

			result.resize(header.ImageCount);
			for (I32 i = 0; i < header.ImageCount; i++) {
				fread_s(&result[i].ICOEntry, sizeof(ICOImageEntry), sizeof(ICOImageEntry), 1, f);
				result[i].Data.resize(result[i].ICOEntry.ImageSize);
			}

			for (ICOImage& image : result) {
				fseek(f, image.ICOEntry.ImageOffset, SEEK_SET);
				fread_s(image.Data.data(), image.ICOEntry.ImageSize, image.ICOEntry.ImageSize, 1, f);
			}

			fclose(f);
		}

		return result;
	}
}