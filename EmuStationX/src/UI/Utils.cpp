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
}