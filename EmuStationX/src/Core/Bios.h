#pragma once

#include <string_view>
#include <vector>
#include <fstream>

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {


	class Bios : public BusDevice {
	public:
		Bios(const String& path);
		~Bios();

		virtual void load(const String& busName, U32 address, U32& output) override;
		virtual void load(const String& busName, U32 address, U8& output) override;

	private:
		Vector<U8> mMemory;
	};

}