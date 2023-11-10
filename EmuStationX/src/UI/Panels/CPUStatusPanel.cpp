#include "UI/Panels/CPUStatusPanel.h"

#include <imgui.h>

namespace esx {



	CPUStatusPanel::CPUStatusPanel()
		:	Panel("CPU Status", false),
			mInstance(nullptr)
	{}

	CPUStatusPanel::~CPUStatusPanel()
	{
	}


	void CPUStatusPanel::onImGuiRender() {
		constexpr static const char* registersMnemonics[] = {
			"$zero",
			"$at",
			"$v0","$v1",
			"$a0","$a1","$a2","$a3",
			"$t0","$t1","$t2","$t3","$t4","$t5","$t6","$t7",
			"$s0","$s1","$s2","$s3","$s4","$s5","$s6","$s7",
			"$t8","$t9",
			"$k0","$k1",
			"$gp",
			"$sp",
			"$fp",
			"$ra"
		};

		float availWidth = ImGui::GetContentRegionAvail().x;

		if (ImGui::BeginTable("CPUStatusTable", 3, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Hideable)) {
			ImGui::TableSetupColumn("GPR", ImGuiTableColumnFlags_WidthFixed, availWidth * 0.15f);
			ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, availWidth * 0.50f);
			ImGui::TableHeadersRow();

			for (int i = 0; i < 32; i++) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::TextUnformatted(registersMnemonics[i]);
				ImGui::TableNextColumn();
				ImGui::Text("0x%08X", mInstance->mRegisters[i]);
			}

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("HI");
			ImGui::TableNextColumn();
			ImGui::Text("0x%08X", mInstance->mHI);

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted("LO");
			ImGui::TableNextColumn();
			ImGui::Text("0x%08X", mInstance->mLO);

			ImGui::EndTable();
		}
	}

}