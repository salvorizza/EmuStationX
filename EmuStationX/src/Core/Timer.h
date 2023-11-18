#pragma once

#include <array>

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {


	class Timer : public BusDevice {
	public:
		Timer();
		~Timer();


		virtual void store(const String& busName, U32 address, U16 value);

		virtual void load(const String& busName, U32 address, U32& value);
		virtual void store(const String& busName, U32 address, U32 value);

	};

}