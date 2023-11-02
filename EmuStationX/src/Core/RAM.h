#pragma once

#include <string_view>
#include <vector>
#include <fstream>

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {


	class RAM : public BusDevice {
	public:
		RAM();
		~RAM();


		virtual void store(const String& busName, U32 address, U8 value) override;
		virtual void load(const String& busName, U32 address, U8& output) override;

		virtual void store(const String& busName, U32 address, U32 value) override;
		virtual void load(const String& busName, U32 address, U32& output) override;
	private:
		std::vector<U8> mMemory;
	};

}