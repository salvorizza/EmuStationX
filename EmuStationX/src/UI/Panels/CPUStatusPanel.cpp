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

		constexpr static const char* cop0RegistersMnemonics[] = {
			"N/A",
			"N/A",
			"N/A",
			"BPC",
			"N/A",
			"BDA",
			"JumpDest",
			"DCIC",
			"BadVaddr",
			"BDAM",
			"N/A",
			"BPCM",
			"SR",
			"Cause",
			"EPC",
			"PrID",
			"*Garbage*",
			"*Garbage*",
			"*Garbage*",
			"*Garbage*",
			"*Garbage*",
			"*Garbage*",
			"*Garbage*",
			"*Garbage*",
			"*Garbage*",
			"*Garbage*",
			"*Garbage*",
			"*Garbage*",
			"*Garbage*",
			"*Garbage*",
			"*Garbage*",
			"*Garbage*",
			"N/A",
			"N/A",
			"N/A",
			"N/A",
			"N/A",
			"N/A",
			"N/A",
			"N/A",
			"N/A",
			"N/A",
			"N/A",
			"N/A",
			"N/A",
			"N/A",
			"N/A",
			"N/A",
			"N/A",
			"N/A",
			"N/A",
			"N/A",
			"N/A",
			"N/A",
			"N/A",
			"N/A",
			"N/A",
			"N/A",
			"N/A",
			"N/A",
			"N/A",
			"N/A",
			"N/A",
			"N/A"
		};

		enum class TabItem {
			CPU,CP0,CP2DAT,CP2CNT
		};

		static TabItem tabItem = TabItem::CPU;

		if (ImGui::BeginTabBar("SelectCoprocessor"))
		{
			if (ImGui::BeginTabItem("CPU")) {
				tabItem = TabItem::CPU;
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("CP0")) {
				tabItem = TabItem::CP0;
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("CP2DAT")) {
				tabItem = TabItem::CP2DAT;
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("CP2CNT")) {
				tabItem = TabItem::CP2CNT;
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		float availWidth = ImGui::GetContentRegionAvail().x;
		GTE& gte = mInstance->mGTE;

		auto drawVector = []<typename T>(const char* name, T * vector, U32 size, const char* format = "%d") {
			ImGui::Text("%s:", name);
			ImGui::SameLine(50);
			if (ImGui::BeginTable(name, size, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
				ImGui::TableNextRow();
				ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, IM_COL32(45, 45, 45, 255));
				ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(45, 45, 45, 255));

				for (U32 i = 0; i < size; i++) {
					ImGui::TableNextColumn();
					ImGui::Text(format, vector[i]);
				}

				ImGui::EndTable();
			}
		};

		auto drawMatrix = []<typename T>(const char* name, T* vector, U32 sizeX, U32 sizeY) {
			float tempY = ImGui::GetCursorPosY();
			ImGui::SetCursorPosY(tempY + 20);
			ImGui::Text("%s:", name);
			ImGui::SameLine(50);
			ImGui::SetCursorPosY(tempY);
			if (ImGui::BeginTable(name, sizeX, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
				for (U32 y = 0; y < sizeY; y++) {
					ImGui::TableNextRow();
					ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg1, IM_COL32(45, 45, 45, 255));
					ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(45, 45, 45, 255));
					for (U32 x = 0; x < sizeX; x++) {
						ImGui::TableNextColumn();
						ImGui::Text("%d", vector[y * sizeX + x]);
					}
				}

				ImGui::EndTable();
			}
		};

		switch (tabItem) {
			case TabItem::CPU: {
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

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::TextUnformatted("PC");
					ImGui::TableNextColumn();
					ImGui::Text("0x%08X", mInstance->mPC);

					ImGui::EndTable();
				}
				break;
			}

			case TabItem::CP0: {
				if (ImGui::BeginTable("CP0StatusTable", 3, ImGuiTableFlags_SizingStretchSame | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Hideable)) {
					ImGui::TableSetupColumn("Register", ImGuiTableColumnFlags_WidthFixed, availWidth * 0.30f);
					ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, availWidth * 0.50f);
					ImGui::TableHeadersRow();

					for (int i = 0; i < 64; i++) {
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::TextUnformatted(cop0RegistersMnemonics[i]);
						ImGui::TableNextColumn();
						ImGui::Text("0x%08X", mInstance->mCP0Registers[i]);
					}
					
					ImGui::EndTable();
				}
				break;
			}

			case TabItem::CP2DAT: {
				Array<I16, 3> IRV = { I16(gte.mRegisters.IR1),I16(gte.mRegisters.IR2),I16(gte.mRegisters.IR3) };

				drawVector("V0", gte.mRegisters.V0.data(), 3);
				drawVector("V1", gte.mRegisters.V1.data(), 3);
				drawVector("V2", gte.mRegisters.V2.data(), 3);
				drawVector("RGBC", gte.mRegisters.RGBC.data(), 4);
				drawVector("OTZ", &gte.mRegisters.OTZ, 1);
				drawVector("IR0", &gte.mRegisters.IR0, 1);
				drawVector("IRV", IRV.data(), 3);
				drawVector("SXY0", gte.mRegisters.SXY0.data(), 2);
				drawVector("SXY1", gte.mRegisters.SXY1.data(), 2);
				drawVector("SXY2", gte.mRegisters.SXY2.data(), 2);
				drawVector("SZ0", &gte.mRegisters.SZ0, 1);
				drawVector("SZ1", &gte.mRegisters.SZ1, 1);
				drawVector("SZ2", &gte.mRegisters.SZ2, 1);
				drawVector("SZ3", &gte.mRegisters.SZ3, 1);
				drawVector("RGB0", gte.mRegisters.RGB0.data(), 4);
				drawVector("RGB1", gte.mRegisters.RGB1.data(), 4);
				drawVector("RGB2", gte.mRegisters.RGB2.data(), 4);
				drawVector("MAC0", &gte.mRegisters.MAC0, 1);
				drawVector("MACV", gte.mRegisters.MACV.data(), 3);
				drawVector("IRGB", &gte.mRegisters.IRGB, 1);
				drawVector("ORGB", &gte.mRegisters.ORGB, 1);
				drawVector("LZCS", &gte.mRegisters.LZCS, 1);
				drawVector("LZCR", &gte.mRegisters.LZCR, 1);

				break;
			}

			case TabItem::CP2CNT: {
				Array<I16, 3> IRV = { I16(gte.mRegisters.IR1),I16(gte.mRegisters.IR2),I16(gte.mRegisters.IR3) };

				drawMatrix("RT", gte.mRegisters.RT.data(), 3, 3);
				drawVector("TR", gte.mRegisters.TR.data(), 3);
				drawMatrix("LLM", gte.mRegisters.LLM.data(), 3, 3);
				drawVector("BK", gte.mRegisters.BK.data(), 3);
				drawMatrix("LCM", gte.mRegisters.LCM.data(), 3, 3);
				drawVector("FC", gte.mRegisters.FC.data(), 3);
				drawVector("OF", gte.mRegisters.OF.data(), 3);
				drawVector("H", &gte.mRegisters.H, 1);
				drawVector("DQA", &gte.mRegisters.DQA, 1);
				drawVector("DQB", &gte.mRegisters.DQB, 1);
				drawVector("ZSF3", &gte.mRegisters.ZSF3, 1);
				drawVector("ZSF4", &gte.mRegisters.ZSF4, 1);
				drawVector("FLAG", &gte.mRegisters.FLAG, 1, "0x%08X");

				break;
			}
		}

	}

}