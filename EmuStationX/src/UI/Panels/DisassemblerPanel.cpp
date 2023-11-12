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
		address = address & SEGS_MASKS[address >> 29];
		auto it = std::find_if(mBreakpoints.begin(), mBreakpoints.end(), [&](Breakpoint& b) { return b.Address == address; });
		if (it != mBreakpoints.end()) {
			disassemble(address - 4 * disassembleRange, 4 * disassembleRange * 2);

			setDebugState(DebugState::Breakpoint);
			mScrollToCurrent = true;
			mCurrent = address;
			return true;
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
				for (int i = 0; i < 100; i++) {
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

				disassemble(mInstance->mPC - 4 * disassembleRange, 4 * disassembleRange * 2);
				mScrollToCurrent = true;
				mCurrent = mInstance->mPC;
				setDebugState(DebugState::Breakpoint);
				break;

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

		if (ImGui::Button(ICON_FA_PLAY)) onPlay();
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_STOP)) onStop();
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_STEP_FORWARD)) onStepForward();
		ImGui::SameLine();

		switch (mDebugState)
		{
			case esx::DisassemblerPanel::DebugState::Idle:
				ImGui::TextUnformatted("Idle");
				break;
			case esx::DisassemblerPanel::DebugState::Start:
				ImGui::TextUnformatted("Start");
				break;
			case esx::DisassemblerPanel::DebugState::Running:
				ImGui::TextUnformatted("Running");
				break;
			case esx::DisassemblerPanel::DebugState::Breakpoint:
				ImGui::TextUnformatted("Breakpoint");
				break;
			case esx::DisassemblerPanel::DebugState::Step:
				ImGui::TextUnformatted("Step");
				break;
			case esx::DisassemblerPanel::DebugState::Stop:
				ImGui::TextUnformatted("Stop");
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
		if (ImGui::BeginTable("Disassembly Table", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders, ImVec2(0,sizeY * 0.75f))) {
			ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, addressingSize);
			ImGui::TableSetupColumn("Mnemonic", ImGuiTableColumnFlags_WidthFixed, contentCellsWidth);
			ImGui::TableHeadersRow();

			ImGuiListClipper clipper;
			clipper.Begin(mInstructions.size());
			while (clipper.Step())
			{
				for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
					Instruction& instruction = mInstructions[row];
					U32 address = instruction.Address;

					ImGui::TableNextRow();
					if (mScrollToCurrent && address == mCurrent) {
						ImGui::SetScrollHereY(0.75);
						mScrollToCurrent = false;
					}

					if (mDebugState != DebugState::Idle && address == mInstance->mPC) {
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
			I64 indexToModify = -1;
			U32 newAddress = 0;
			I64 i = 0;
			static char addressBuffer[32];
			for (const auto& breakpoint : mBreakpoints) {
				ImGui::TableNextRow();

				ImGui::TableNextColumn();
				ImGui::Button(ICON_FA_CIRCLE);

				ImGui::TableNextColumn();
				sprintf_s(addressBuffer, 32, "0x%08X", breakpoint.Address);
				ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(1, 1, 1, 0));
				ImGui::PushID(i);
				if (ImGui::InputText("##goto", addressBuffer, IM_ARRAYSIZE(addressBuffer), ImGuiInputTextFlags_CharsHexadecimal))
				{
					U32 addr;
					if (sscanf(addressBuffer, "0x%08X", &addr) == 1) {
						newAddress = addr;
						indexToModify = i;
					} else if (sscanf(addressBuffer, "%08X", &addr) == 1) {
						newAddress = addr;
						indexToModify = i;
					}
				}
				ImGui::PopID();
				ImGui::PopStyleColor(1);

				ImGui::TableNextColumn();
				if (ImGui::Button(ICON_FA_TRASH)) {
					indexToDelete = i;
				}

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

			if (indexToModify != -1) {
				mBreakpoints[indexToModify].Address = newAddress;
			}

			ImGui::EndTable();
		}
	}

	void DisassemblerPanel::disassemble(uint32_t startAddress, size_t size){
		mInstructions.clear();
		for (U32 address = startAddress; address < startAddress + size; address +=4) {
			address = address & SEGS_MASKS[address >> 29];

			if ((address >= 0x00000000 && address <= KIBI(2048)) || (address >= 0x1FC00000 && address < 0x1FC00000 + KIBI(512))) {

				U32 opcode = mInstance->getBus(ESX_TEXT("Root"))->load<U32>(address);
				auto cpuInstruction = mInstance->decode(opcode, address);

				Instruction instruction;
				instruction.Address = address;
				instruction.Mnemonic = std::string(cpuInstruction.Mnemonic.begin(), cpuInstruction.Mnemonic.end());
				mInstructions.push_back(instruction);
			}

		}
	}

	void DisassemblerPanel::onPlay() {
		switch (mDebugState) {
			case DebugState::Idle:
				setDebugState(DebugState::Start);
				break;

			default:
				setDebugState(DebugState::Running);
				break;
		}
	}

	void DisassemblerPanel::onStop() {
		setDebugState(DebugState::Stop);
	}

	void DisassemblerPanel::onStepForward() {
		if (mDebugState == DebugState::Breakpoint) {
			setDebugState(DebugState::Step);
		}
	}

}