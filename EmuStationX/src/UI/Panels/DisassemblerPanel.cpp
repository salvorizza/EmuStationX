#include "UI/Panels/DisassemblerPanel.h"

#include "UI/Window/FontAwesome5.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>
#include <imgui.h>

namespace esx {



	DisassemblerPanel::DisassemblerPanel()
		:	Panel("Disassembler", false),
			mInstance(nullptr),
			mDebugState(DebugState::Idle),
			mPrevDebugState(DebugState::None),
			mScrollToCurrent(false)
	{
	}

	DisassemblerPanel::~DisassemblerPanel()
	{
	}

	bool DisassemblerPanel::breakFunction(U32 address)
	{
		if (mBreakpoints.size() > 0) {
			auto it = std::find_if(mBreakpoints.begin(), mBreakpoints.end(), [&](Breakpoint& b) { return R3000::toPhysicalAddress(b.Address) == R3000::toPhysicalAddress(address) && b.Enabled; });
			if (it != mBreakpoints.end()) {
				//disassemble(address - 4 * disassembleRange, 4 * disassembleRange * 2);

				setDebugState(DebugState::Breakpoint);
				mScrollToCurrent = true;
				mCurrent = address;
				return true;
			}
		}
		return false;

	}


	void DisassemblerPanel::onUpdate()
	{
		switch (mDebugState) {
			case DebugState::Start:
				setDebugState(DebugState::Running);
				break;

			case DebugState::Running:
				for (int i = 0; i < 1000000; i++) {
					if (breakFunction(mInstance->mPC)) {
						setDebugState(DebugState::Breakpoint);
						break;
					} else {
						mInstance->clock();
					}
				}
				break;

			case DebugState::Step:
				mInstance->clock();

				//disassemble(mInstance->mPC - 4 * disassembleRange, 4 * disassembleRange * 2);
				mScrollToCurrent = true;
				mCurrent = mInstance->mPC;
				setDebugState(DebugState::Breakpoint);
				break;


			case DebugState::StepOver: {
				for (int i = 0; i < 1000000; i++) {
					if (mInstance->mPC == mNextPC) {
						mScrollToCurrent = true;
						mCurrent = mInstance->mPC;
						setDebugState(DebugState::Breakpoint);
						break;
					}
					else {
						mInstance->clock();
					}
				}
				break;
			}


			case DebugState::Stop:
				setDebugState(DebugState::Idle);
				break;
		}
	}

	void DisassemblerPanel::onImGuiRender()
	{

		float availWidth = ImGui::GetContentRegionAvail().x;
		float oneCharSize = ImGui::CalcTextSize("A").x;
		float addressingSize = oneCharSize * 11;
		float contentCellsWidth = availWidth - (addressingSize);

		if (mDebugState == DebugState::Idle || mDebugState == DebugState::Breakpoint) {
			if (ImGui::Button(ICON_FA_PLAY))
				onPlay();
		}
		else {
			if (ImGui::Button(ICON_FA_PAUSE)) {
				onPause();
			}
		}
		if (mDebugState == DebugState::Breakpoint) {
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_STEP_FORWARD)) onStepForward();
			ImGui::SameLine();
			if (ImGui::Button(ICON_FA_ARROWS_TURN_DOWN)) onStepOver();
		}
		
		switch (mDebugState)
		{
			case esx::DisassemblerPanel::DebugState::Idle:
				ImGui::TextUnformatted("State: Idle");
				break;
			case esx::DisassemblerPanel::DebugState::Start:
				ImGui::TextUnformatted("State: Start");
				break;
			case esx::DisassemblerPanel::DebugState::Running:
				ImGui::TextUnformatted("State: Running");
				break;
			case esx::DisassemblerPanel::DebugState::Breakpoint:
				ImGui::TextUnformatted("State: Breakpoint");
				break;
			case esx::DisassemblerPanel::DebugState::Step:
				ImGui::TextUnformatted("State: Step");
				break;
			case esx::DisassemblerPanel::DebugState::Stop:
				ImGui::TextUnformatted("State: Stop");
				break;
			default:
				break;
		}

		if (ImGui::BeginTabBar("SelectDisassembleRom"))
		{
			if (ImGui::BeginTabItem("Instructions")) {
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		static bool p_open = true;
		float sizeY = ImGui::GetContentRegionAvail().y;


		if (ImGui::BeginChild("##child", ImVec2(0, sizeY * 0.75f))) {
			if (ImGui::BeginTable("Disassembly Table", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
				ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, addressingSize);
				ImGui::TableSetupColumn("Mnemonic", ImGuiTableColumnFlags_WidthFixed, contentCellsWidth);
				ImGui::TableHeadersRow();

				constexpr size_t numInstructionsRAM = 0x00800000 / 4;
				constexpr size_t numInstructionsBios = 0x00080000 / 4;

				ImGuiListClipper clipper;
				clipper.Begin(numInstructionsRAM + numInstructionsBios);
				if (mScrollToCurrent) {
					U32 index = 0;

					mCurrent = R3000::toPhysicalAddress(mCurrent);
					
					if (mCurrent >= 0x1FC00000) {
						index = numInstructionsRAM + (mCurrent - 0x1FC00000) / 4;
					} else if (mCurrent >= 0x00000000) {
						index = (mCurrent - 0x00000000) / 4;
					} 
					clipper.ForceDisplayRangeByIndices(index, index + 1);
				}
				while (clipper.Step())
				{
					for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
						esx::Instruction cpuInstruction;
						
						U32 physAddress = row * 4;
						U32 translatedAddress = 0x00000000 + physAddress;
						if (row >= numInstructionsRAM) {
							physAddress = 0x1FC00000 + (row - numInstructionsRAM) * 4;
							translatedAddress = 0x1FC00000 + (row - numInstructionsRAM) * 4;
						}

						U32 opcode = mInstance->getBus(ESX_TEXT("Root"))->load<U32>(physAddress);
						mInstance->decode(cpuInstruction, opcode, physAddress, ESX_TRUE);

						Instruction instruction;
						instruction.Address = translatedAddress;
						instruction.Mnemonic = cpuInstruction.Mnemonic();

						//Instruction instruction = mInstructions[row];

						U32 address = R3000::toPhysicalAddress(instruction.Address);

						ImGui::TableNextRow();
						if (mScrollToCurrent && address == mCurrent) {
							ImGui::SetScrollHereY(0.75);
							mScrollToCurrent = false;
						}

						if (mDebugState != DebugState::Idle && address == R3000::toPhysicalAddress(mInstance->mPC)) {
							ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(230, 100, 120, 125));
							ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, IM_COL32(180, 50, 70, 125));
						}

						ImGui::TableNextColumn();
						ImGui::Text("0x%08X", address);

						ImGui::TableNextColumn();
						ImGui::Text(instruction.Mnemonic.c_str());
					}
				}
				clipper.End();
				ImGui::EndTable();
			}
			ImGui::EndChild();
		}
		
		if (ImGui::BeginTabBar("SelectDisassembleRom2"))
		{
			if (ImGui::BeginTabItem("Breakpoints")) {
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		sizeY = ImGui::GetContentRegionAvail().y;
		contentCellsWidth = availWidth - (oneCharSize * 11);
		if (ImGui::BeginTable("Breakpoints Table", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders, ImVec2(0, sizeY))) {
			ImGui::TableSetupColumn("Break", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHeaderLabel, oneCharSize * 3);
			ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, contentCellsWidth);
			ImGui::TableSetupColumn("Delete", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoHeaderLabel, oneCharSize * 3);
			ImGui::TableHeadersRow();

			I64 indexToDelete = -1;
			int i = 0;
			static char addressBuffer[32];
			for (auto& breakpoint : mBreakpoints) {
				ImGui::TableNextRow();

				ImGui::TableNextColumn();
				if (breakpoint.Enabled) {
					if (ImGui::Button(ICON_FA_EYE)) breakpoint.Enabled = false;
				} else {
					if (ImGui::Button(ICON_FA_EYE_SLASH)) breakpoint.Enabled = true;
				}

				ImGui::TableNextColumn();
				sprintf_s(addressBuffer, 32, "0x%08X", breakpoint.Address);
				ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1, 1, 1, 0));
				ImGui::PushID(i);
				if (ImGui::InputText("##goto", addressBuffer, IM_ARRAYSIZE(addressBuffer), ImGuiInputTextFlags_CharsHexadecimal))
				{
					U32 addr;
					if (sscanf_s(addressBuffer, "0x%08X", &addr) == 1 || sscanf_s(addressBuffer, "%08X", &addr) == 1) {
						breakpoint.Address = addr;
					}
				}
				ImGui::PopID();
				ImGui::PopStyleColor(1);

				ImGui::TableNextColumn();
				ImGui::PushID(i);
				if (ImGui::Button(ICON_FA_TRASH)) {
					indexToDelete = i;
				}
				ImGui::PopID();

				i++;
			}

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TableNextColumn();
			ImGui::TableNextColumn();
			if (ImGui::Button(ICON_FA_PLUS)) {
				mBreakpoints.push_back(Breakpoint());
			}

			if (indexToDelete != -1) {
				mBreakpoints.erase(mBreakpoints.begin() + indexToDelete);
			}

			ImGui::EndTable();
		}
	}

	void DisassemblerPanel::disassemble(uint32_t startAddress, size_t size){
		esx::Instruction cpuInstruction;

		mInstructions.clear();
		for (U32 address = startAddress; address < startAddress + size; address +=4) {
			U32 physAddress = address & SEGS_MASKS[address >> 29];

			if ((physAddress >= 0x00000000 && physAddress <= KIBI(2048)) || (physAddress >= 0x1FC00000 && physAddress < 0x1FC00000 + KIBI(512))) {

				U32 opcode = mInstance->getBus(ESX_TEXT("Root"))->load<U32>(physAddress);
				mInstance->decode(cpuInstruction, opcode, physAddress, ESX_TRUE);

				Instruction instruction;
				instruction.Address = address;
				instruction.Mnemonic = cpuInstruction.Mnemonic();
				mInstructions.push_back(instruction);
			}

		}
	}

	void DisassemblerPanel::onPlay() {
		switch (mDebugState) {
			case DebugState::Idle:
				setDebugState(DebugState::Start);
				break;

			case DebugState::Breakpoint:
				mInstance->clock();
			default:
				setDebugState(DebugState::Running);
				break;
		}
	}

	void DisassemblerPanel::onPause() {
		setDebugState(DebugState::Breakpoint);
		//disassemble(mInstance->mPC - 4 * disassembleRange, 4 * disassembleRange * 2);
		mScrollToCurrent = true;
		mCurrent = mInstance->mPC;
	}

	void DisassemblerPanel::onStepForward() {
		if (mDebugState == DebugState::Breakpoint) {
			setDebugState(DebugState::Step);
		}
	}

	void DisassemblerPanel::onStepOver()
	{
		if (mDebugState == DebugState::Breakpoint) {
			mNextPC = mInstance->mPC + 4;
			setDebugState(DebugState::StepOver);
		}
	}

}