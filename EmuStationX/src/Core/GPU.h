#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {


	class GPU : public BusDevice {
	public:
		GPU();
		~GPU();

		virtual void store(const String& busName, U32 address, U32 value) override;
		virtual void load(const String& busName, U32 address, U32& output) override;
	};

}