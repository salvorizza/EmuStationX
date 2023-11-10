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
			mScrollToCurrent(false),
			mDebugTab(DebugTab::Bios),
			mSelectTab(DebugTab::None)
	{
	}

	DisassemblerPanel::~DisassemblerPanel()
	{
	}

	void DisassemblerPanel::disassembleBios()
	{
		mInstructions.clear();
		mInstructionsKeys.clear();

		disassemble(DebugTab::Bios,mInstructions, 0x1FC00000, KIBI(512));
		for (auto& [address, debugInstruction] : mInstructions)
			mInstructionsKeys.push_back(address);
	}

	bool DisassemblerPanel::breakFunction(uint32_t address)
	{
		auto it = mInstructionsBreaks.find(std::pair<DebugTab, uint32_t>(mDebugTab, address));
		if (it != mInstructionsBreaks.end()) {
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
				mScrollToCurrent = true;
				mCurrent = mInstance->mPC;
				setDebugState(DebugState::Breakpoint);
				break;

			case DebugState::Stop:
				setDebugState(DebugState::Idle);
				break;
		}
	}

	void DisassemblerPanel::search(const std::string& key)
	{
		mSearchResults.clear();

		if (key.empty()) {
			return;
		}

		auto& instructionMap = getCurrentInstructionMap();
		for (const auto& pair : instructionMap) {
			size_t offset = pair.second.Instruction.find(key);
			if (offset != pair.second.Instruction.npos) {
				mSearchResults[pair.first] = offset;
			}
		}
		mSearchResultsIterator = mSearchResults.begin();
	}

	void DisassemblerPanel::onImGuiRender()
	{

		float availWidth = ImGui::GetContentRegionAvail().x;
		float oneCharSize = ImGui::CalcTextSize("A").x;
		float bulletSize = oneCharSize * 2;
		float lineSize = oneCharSize * 8;
		float addressingSize = oneCharSize * 11;
		float contentCellsWidth = availWidth - (bulletSize + addressingSize + lineSize);

		if (ImGui::Button(ICON_FA_PLAY)) onPlay();
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_STOP)) onStop();
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_STEP_FORWARD)) onStepForward();
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_SYNC)) {
			disassembleBios();
		}
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


		ImGui::Text("Search:");
		ImGui::SameLine();
		static char searchBuffer[32];
		if (ImGui::InputText("##search", searchBuffer, 32)) {
			search(searchBuffer);
			if (!mSearchResults.empty()) {
				mScrollToCurrent = true;
				mCurrent = mSearchResultsIterator->first;
			}
		}
		ImVec2 searchTextUISize = ImGui::CalcTextSize(searchBuffer);


		ImGui::SameLine();

		bool condition = mSearchResults.empty();
		if (condition) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.14f, 0.14f, 0.14f, 0.0f));
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.14f, 0.14f, 0.0f));
		}
		ImGui::PushItemFlag(ImGuiItemFlags_Disabled, condition);

		if (ImGui::Button(ICON_FA_ARROW_RIGHT)) {
			mSearchResultsIterator++;
			if (mSearchResultsIterator == mSearchResults.end()) {
				mSearchResultsIterator = mSearchResults.begin();
			}

			mScrollToCurrent = true;
			mCurrent = mSearchResultsIterator->first;
		}

		ImGui::PopItemFlag();

		if (condition) {
			ImGui::PopStyleColor(3);
		}

		ImGui::Text("Go To:  ");
		ImGui::SameLine();
		static char gotoBuffer[32];
		if (ImGui::InputText("##goto", gotoBuffer, IM_ARRAYSIZE(gotoBuffer), ImGuiInputTextFlags_CharsHexadecimal))
		{
			uint32_t gotoAddr;
			if (sscanf(gotoBuffer, "%08X", &gotoAddr) == 1) {
				mScrollToCurrent = true;
				mCurrent = gotoAddr;
			}
		}
		

		static bool p_open = true;
		static bool p_open_2 = true;

		if (ImGui::BeginTabBar("SelectDisassembleRom"))
		{
			ImGuiTabItemFlags flag = ImGuiTabItemFlags_None;
			if (mSelectTab == DebugTab::Bios) {
				flag = ImGuiTabItemFlags_SetSelected;
				mSelectTab = DebugTab::None;
			}

			if (ImGui::BeginTabItem("Bios", &p_open, flag))
			{
				mDebugTab = DebugTab::Bios;
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::BeginChild("Disas");
		if (ImGui::BeginTable("Disassembly Table", 4, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, bulletSize);
			ImGui::TableSetupColumn("Line", ImGuiTableColumnFlags_WidthFixed, lineSize);
			ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, addressingSize);
			ImGui::TableSetupColumn("Instruction", ImGuiTableColumnFlags_WidthFixed, contentCellsWidth);
			ImGui::TableHeadersRow();

		

			uint32_t i = 1;
			auto& instructionMap = getCurrentInstructionMap();
			auto& keys = getCurrentInstructionMapKeys();
			ImGuiListClipper clipper;

			clipper.Begin(instructionMap.size());

			if (mScrollToCurrent) {
				auto it = std::find(keys.begin(), keys.end(), mCurrent);
				size_t val = std::distance(keys.begin(), it);
				clipper.ForceDisplayRangeByIndices(val - 1, val + 1);
			}

			while (clipper.Step())
			{
				for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
					auto it = mInstructions.find(keys[row]);

					uint32_t address = it->first;
					DebugInstruction& debugInstruction = it->second;

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
					ImGui::PushStyleColor(ImGuiCol_Button, debugInstruction.Breakpoint ? ImVec4(1, 0, 0, 1) : ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, debugInstruction.Breakpoint ? ImVec4(1, 0, 0, 1) : ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, debugInstruction.Breakpoint ? ImVec4(1, 0, 0, 1) : ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0,0));
					ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,50);
					ImGui::PushID(row + 1);
					if (ImGui::Button("",ImVec2(14, 14))) {
						debugInstruction.Breakpoint = !debugInstruction.Breakpoint;
						
						if (debugInstruction.Breakpoint) {
							mInstructionsBreaks.emplace(std::pair<DebugTab, uint32_t>(mDebugTab, address), debugInstruction);
						} else {
							mInstructionsBreaks.erase(std::pair<DebugTab, uint32_t>(mDebugTab, address));
						}
					}
					ImGui::PopID();
					ImGui::PopStyleColor(3);
					ImGui::PopStyleVar(2);

					ImGui::TableNextColumn();
					ImGui::Text("%04u", row + 1);

					ImGui::TableNextColumn();
					ImGui::Text("0x%08X", address);

					ImGui::TableNextColumn();
					ImVec2 cursorPos = ImGui::GetCursorPos();
					if (strlen(searchBuffer) != 0) {
						auto searchIterator = mSearchResults.find(keys[row]);
						if (searchIterator != mSearchResults.end()) {
							std::string subString = debugInstruction.Instruction.substr(0, searchIterator->second);
							ImVec2 subStringUIOffset = ImGui::CalcTextSize(subString.c_str());
							ImVec2 windowPos = ImGui::GetWindowPos();
							ImVec2 min = ImVec2(windowPos.x + cursorPos.x + subStringUIOffset.x, windowPos.y + cursorPos.y - ImGui::GetScrollY());
							ImVec2 max = ImVec2(windowPos.x + cursorPos.x + subStringUIOffset.x + searchTextUISize.x, windowPos.y + cursorPos.y + searchTextUISize.y - ImGui::GetScrollY());
							
							ImU32 color = IM_COL32(0, 112, 224, 100);
							if (mSearchResultsIterator->first == keys[row]) {
								color = IM_COL32(0, 112, 224, 255);
							}

							ImGui::GetWindowDrawList()->AddRectFilled(min,max, color);
						}
					}
					ImGui::Text("%s", debugInstruction.Instruction.c_str());
				}
			}
			clipper.End();
			ImGui::EndTable();
		}
		ImGui::EndChild();
	}

	void DisassemblerPanel::disassemble(DebugTab tab, std::map<uint32_t, DisassemblerPanel::DebugInstruction>& instructionsMap,uint32_t startAddress, size_t size){
		for (size_t address = startAddress; address < startAddress + size; address +=4) {
			U32 opcode = mInstance->getBus(ESX_TEXT("Root"))->load<U32>(address);
			auto instruction = mInstance->decode(opcode);
			auto pair = std::pair<DebugTab, uint32_t>(tab, instruction.Address);
			std::string str(instruction.Mnemonic.begin(), instruction.Mnemonic.end());

			DebugInstruction di(str, mInstructionsBreaks.find(pair) != mInstructionsBreaks.end());
			instructionsMap[address] = di;
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