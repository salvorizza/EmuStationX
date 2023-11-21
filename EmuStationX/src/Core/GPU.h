#pragma once

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {


	class GPU : public BusDevice {
	public:
		GPU();
		~GPU();

		virtual void store(const StringView& busName, U32 address, U32 value) override;
		virtual void load(const StringView& busName, U32 address, U32& output) override;
	};

}