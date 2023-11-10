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

		void setInstance(R3000* pInstance) { mInstance = pInstance;}

		void disassembleBios();

		bool breakFunction(uint32_t address);

		void onUpdate();

	private:
		void search(const std::string& key);

	protected:
		virtual void onImGuiRender() override;

	private:
		struct DebugInstruction {
			std::string Instruction;
			bool Breakpoint;

			DebugInstruction()
				: Breakpoint(false),
				Instruction()
			{}

			DebugInstruction(const std::string& instruction, bool breakpoint = false)
				: Breakpoint(breakpoint),
				Instruction(instruction)
			{}
		};

		enum class DebugState {
			None,
			Idle,
			Start,
			Running,
			Breakpoint,
			Step,
			Stop
		};

		enum class DebugTab {
			None,
			Bios
		};

	private:
		void disassemble(DebugTab tab,std::map<uint32_t, DisassemblerPanel::DebugInstruction>& instructionMap, uint32_t startAddress, size_t size);
		inline std::map<uint32_t, DisassemblerPanel::DebugInstruction>& getCurrentInstructionMap() { return mInstructions; }
		inline std::vector<uint32_t>& getCurrentInstructionMapKeys() { return mInstructionsKeys; }

		void onPlay();
		void onStop();
		void onStepForward();

		void setDebugState(DebugState debugState) { mPrevDebugState = mDebugState; mDebugState = debugState; }

		R3000* mInstance;

		std::map<std::pair<DebugTab, uint32_t>, DisassemblerPanel::DebugInstruction> mInstructionsBreaks;


		std::map<uint32_t, DisassemblerPanel::DebugInstruction> mInstructions;
		std::vector<uint32_t> mInstructionsKeys;

		std::map<uint32_t, size_t> mSearchResults;
		std::map<uint32_t, size_t>::iterator mSearchResultsIterator;

		DebugState mDebugState;
		DebugState mPrevDebugState;
		DebugTab mDebugTab,mSelectTab;

		bool mScrollToCurrent;
		uint32_t mCurrent;
	};

}
