#pragma once

#include <string_view>
#include <vector>
#include <fstream>

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {


	class Bios : public BusDevice {
		friend class MemoryEditorPanel;
	public:
		Bios(const StringView& path);
		~Bios();

		virtual void load(const StringView& busName, U32 address, U32& output) override;
		virtual void load(const StringView& busName, U32 address, U16& output) override;
		virtual void load(const StringView& busName, U32 address, U8& output) override;


		virtual void reset();

		virtual U8* getFastPointer(U32 address) override;

	private:
		Vector<U8> mMemory;
		StringView mPath;
	};

}