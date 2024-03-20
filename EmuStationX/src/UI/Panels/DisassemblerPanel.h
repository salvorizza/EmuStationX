#pragma once

#include <UI/Panels/Panel.h>
#include <Core/R3000.h>

#include <map>
#include <vector>

namespace esx {

	class DisassemblerPanel : public Panel {
	public:
		DisassemblerPanel();
		~DisassemblerPanel();

		void setInstance(const SharedPtr<R3000>& pInstance) { mInstance = pInstance;}

		bool breakFunction(U32 address);

		void onUpdate();

	protected:
		virtual void onImGuiRender() override;

	private:
		struct Instruction {
			U32 Address;
			std::string Mnemonic;
		};

		struct Breakpoint {
			bool Enabled = true;
			U32 Address;
		};

		enum class DebugState {
			None,
			Idle,
			Start,
			Running,
			Breakpoint,
			Step,
			StepOver,
			Stop
		};

	private:
		void disassemble(uint32_t startAddress, size_t size);

		void onPlay();
		void onPause();
		void onStepForward();
		void onStepOver();

		void setDebugState(DebugState debugState) { mPrevDebugState = mDebugState; mDebugState = debugState; }

		SharedPtr<R3000> mInstance;

		std::vector<Instruction> mInstructions;
		std::vector<Breakpoint> mBreakpoints;

		DebugState mDebugState;
		DebugState mPrevDebugState;
		bool mScrollToCurrent;
		uint32_t mCurrent;
		U32 mNextPC;

		static const size_t disassembleRange = 10;
	};

}
