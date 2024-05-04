#include "UI/Panels/SPUStatusPanel.h"

#include <imgui.h>

namespace esx {



	SPUStatusPanel::SPUStatusPanel()
		: Panel("SPU Status", false),
		mInstance(nullptr)
	{}

	SPUStatusPanel::~SPUStatusPanel()
	{
	}


	void SPUStatusPanel::onImGuiRender() {
		
		if (ImGui::BeginTable("SPUVoiceStatusTable", 12)) {
			ImGui::TableSetupColumn("#");
			ImGui::TableSetupColumn("InterpIndex");
			ImGui::TableSetupColumn("SampleIndex");
			ImGui::TableSetupColumn("CurAddr");
			ImGui::TableSetupColumn("StartAddr");
			ImGui::TableSetupColumn("RepeatAddr");
			ImGui::TableSetupColumn("SampleRate");
			ImGui::TableSetupColumn("VolLeft");
			ImGui::TableSetupColumn("VolRight");
			ImGui::TableSetupColumn("ADSRPhase");
			ImGui::TableSetupColumn("ADSRVol");
			ImGui::TableSetupColumn("ADSRTick");
			ImGui::TableHeadersRow();

			for (const Voice& voice : mInstance->mVoices) {
				ImGui::TableNextRow();

				ImGui::TableNextColumn();
				ImGui::Text("%d",voice.Number);
				
				ImGui::TableNextColumn();
				ImGui::Text("%d", voice.getInterpolationIndex());

				ImGui::TableNextColumn();
				ImGui::Text("%d", voice.getSampleIndex());

				ImGui::TableNextColumn();
				ImGui::Text("%04X", voice.ADPCMCurrentAddress);

				ImGui::TableNextColumn();
				ImGui::Text("%04X", voice.ADPCMCurrentAddress);

				ImGui::TableNextColumn();
				ImGui::Text("%04X", voice.ADPCMRepeatAddress);

				ImGui::TableNextColumn();
				ImGui::Text("%d", voice.ADPCMSampleRate);

				ImGui::TableNextColumn();
				ImGui::Text("%.2f", ((voice.VolumeLeft.Level + 0x8000) / (float)0xFFFF) * 100);

				ImGui::TableNextColumn();
				ImGui::Text("%.2f", ((voice.VolumeRight.Level + 0x8000) / (float)0xFFFF) * 100);

				ImGui::TableNextColumn();
				switch (voice.ADSR.Phase) {
					case ADSRPhaseType::Attack:
						ImGui::TextUnformatted("Attack");
						break;
					case ADSRPhaseType::Decay:
						ImGui::TextUnformatted("Decay");
						break;
					case ADSRPhaseType::Sustain:
						ImGui::TextUnformatted("Sustain");
						break;
					case ADSRPhaseType::Release:
						ImGui::TextUnformatted("Release");
						break;
					case ADSRPhaseType::Off:
						ImGui::TextUnformatted("Off");
						break;
				}

				ImGui::TableNextColumn();
				ImGui::Text("%.2f", ((voice.ADSR.CurrentVolume) / (float)0x7FFF) * 100);

				ImGui::TableNextColumn();
				ImGui::Text("%d", voice.ADSR.Tick);
			}

			ImGui::EndTable();
		}
	}

}