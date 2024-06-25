#pragma once

#include <string_view>
#include <vector>
#include <fstream>

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {
	class MemoryControl;

	class RAM : public BusDevice {
	public:
		friend class MemoryEditorPanel;
		friend class DisassemblerPanel;

		RAM(const StringView& name,U32 startAddress, U32 addressingSize, U64 size);
		~RAM();

		virtual void init() override;

		virtual void store(const StringView& busName, U32 address, U8 value) override;
		virtual void load(const StringView& busName, U32 address, U8& output) override;

		virtual void store(const StringView& busName, U32 address, U16 value) override;
		virtual void load(const StringView& busName, U32 address, U16& output) override;

		virtual void store(const StringView& busName, U32 address, U32 value) override;
		virtual void load(const StringView& busName, U32 address, U32& output) override;

		virtual void reset() override;

	private:
		void checkLocked(U32 address);
		BIT isHiZ(U32 address);
	private:
		Vector<U8> mMemory;
		SharedPtr<MemoryControl> mMemoryControl;
	};

}