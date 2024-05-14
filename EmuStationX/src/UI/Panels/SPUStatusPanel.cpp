#include "UI/Panels/SPUStatusPanel.h"

#include <imgui.h>
#include "MemoryEditor.h"

namespace esx {



	SPUStatusPanel::SPUStatusPanel()
		: Panel("SPU State", false),
		mInstance(nullptr)
	{}

	SPUStatusPanel::~SPUStatusPanel()
	{
	}


	void SPUStatusPanel::onImGuiRender() {
		static MemoryEditor mem_edit;

		const ImVec4 kDisabled = ImVec4(0.5, 0.5, 0.5, 1.0);
		const ImVec4 kEnabled = ImVec4(1, 1, 1, 1.0);

		if (ImGui::CollapsingHeader("Status")) {
			ImGui::TextUnformatted("Control:");
			{
				ImGui::SameLine();
				ImGui::TextColored(mInstance->mSPUControl.Enable ? kEnabled : kDisabled, "SPU Enable");

				ImGui::SameLine();
				ImGui::TextColored(!mInstance->mSPUControl.Unmute ? kEnabled : kDisabled, "Mute SPU");

				ImGui::SameLine();
				ImGui::TextColored(mInstance->mSPUControl.ExternalAudioEnable ? kEnabled : kDisabled, "External Audio");

				ImGui::SameLine();
				ImGui::TextColored(mInstance->mSPUControl.TransferMode == TransferMode::Stop ? kEnabled : kDisabled, "Transfer Stopped");
			}

			ImGui::TextUnformatted("Status:");
			{
				ImGui::SameLine();
				ImGui::TextColored(mInstance->mSPUStatus.IRQ9Flag ? kEnabled : kDisabled, "IRQ9");

				ImGui::SameLine();
				ImGui::TextColored(mInstance->mSPUStatus.DataTransferDMAReadRequest || mInstance->mSPUStatus.DataTransferDMAWriteRequest ? kEnabled : kDisabled, "DMA Request");

				ImGui::SameLine();
				ImGui::TextColored(mInstance->mSPUStatus.DataTransferDMAReadRequest ? kEnabled : kDisabled, "DMA Read");

				ImGui::SameLine();
				ImGui::TextColored(mInstance->mSPUStatus.DataTransferDMAWriteRequest ? kEnabled : kDisabled, "DMA Write");

				ImGui::SameLine();
				ImGui::TextColored(mInstance->mSPUStatus.DataTransferBusyFlag ? kEnabled : kDisabled, "Transfer Busy");

				ImGui::SameLine();
				ImGui::TextColored(mInstance->mSPUStatus.WriteToSecondHalf ? kEnabled : kDisabled, "Second Capture Buffer");
			}

			ImGui::TextUnformatted("Interrupt:");
			{
				ImGui::SameLine();
				ImGui::TextColored(mInstance->mSPUControl.IRQ9Enable ? kEnabled : kDisabled, 
					mInstance->mSPUControl.IRQ9Enable ? "Enabled" : "Disabled");
			}

			ImGui::TextUnformatted("Volume:");
			{
				ImGui::SameLine();
				ImGui::Text("Left: %.2f%%", ((mInstance->mMainVolumeLeft.Level + 0x8000) / (float)0xFFFF) * 100);

				ImGui::SameLine();
				ImGui::Text("Right: %.2f%%", ((mInstance->mMainVolumeRight.Level + 0x8000) / (float)0xFFFF) * 100);
			}

			ImGui::TextUnformatted("CD Audio:");
			{
				ImGui::SameLine();
				ImGui::TextColored(!mInstance->mSPUControl.CDAudioEnable ? kEnabled : kDisabled, "Disabled");

				ImGui::SameLine();
				ImGui::TextColored(mInstance->mSPUControl.CDAudioEnable ? kEnabled : kDisabled, "Left Volume: %.2f%%", ((mInstance->mCDInputVolume.Left + 0x8000) / (float)0xFFFF) * 100);

				ImGui::SameLine();
				ImGui::TextColored(mInstance->mSPUControl.CDAudioEnable ? kEnabled : kDisabled, "Right Volume: %.2f%%", ((mInstance->mCDInputVolume.Right + 0x8000) / (float)0xFFFF) * 100);
			}

			ImGui::TextUnformatted("Transfer FIFO:");
			{
				ImGui::SameLine();
				ImGui::TextColored(mInstance->mFIFO.size() ? kEnabled : kDisabled, "%d halfwords (%d bytes)", mInstance->mFIFO.size(), mInstance->mFIFO.size() * 2);
			}
		}

		if (ImGui::CollapsingHeader("Voice State")) {
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

					ImGui::PushStyleColor(ImGuiCol_Text, voice.ADSR.Phase == ADSRPhaseType::Off ? kDisabled : kEnabled);

					ImGui::TableNextColumn();
					ImGui::Text("%d", voice.Number);

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
					ImGui::Text("%.2f", (voice.ADSR.CurrentVolume / (float)0x7FFF) * 100);

					ImGui::TableNextColumn();
					ImGui::Text("%d", voice.ADSR.Tick);

					ImGui::PopStyleColor();
				}

				ImGui::EndTable();
			}
		}

		if (ImGui::CollapsingHeader("Reverb")) {
			
			ImGui::TextColored(mInstance->mSPUControl.ReverbMasterEnable ? kEnabled : kDisabled, "Master Enable: %s", mInstance->mSPUControl.ReverbMasterEnable ? "Yes" : "No");

			ImGui::TextUnformatted("Voices Enabled:");
			for (Voice& voice : mInstance->mVoices) {
				ImGui::SameLine();
				ImGui::TextColored(voice.ReverbMode == ReverbMode::ToMixerAndToReverb ? kEnabled : kDisabled, "%d", voice.Number);
			}

			ImGui::TextColored(mInstance->mSPUControl.CDAudioReverb ? kEnabled : kDisabled,"CD Audio Enable: %s", mInstance->mSPUControl.CDAudioEnable ? "Yes" : "No");

			ImGui::TextColored(mInstance->mSPUControl.ExternalAudioReverb ? kEnabled : kDisabled, "External Audio Enable: %s", mInstance->mSPUControl.ExternalAudioEnable ? "Yes" : "No");

			ImGui::Text("Base Address: 0x%08X (%04X)", (mInstance->mReverb[mBASE] << 3), mInstance->mReverb[mBASE]);
			ImGui::Text("Current Address: 0x%08X", mInstance->mCurrentBufferAddress);
			ImGui::Text("Output Volume: Left %.2f%% Right %.2f%%", (mInstance->mReverb[vLOUT] / (float)0xFFFF) * 100, (mInstance->mReverb[vROUT] / (float)0xFFFF) * 100);

			ImGui::TextUnformatted("Pitch Modulation:");
			for (Voice& voice : mInstance->mVoices) {
				ImGui::SameLine();
				ImGui::TextColored(voice.PitchModulation ? kEnabled : kDisabled, "%d", voice.Number);
			}
		}

		if (ImGui::CollapsingHeader("RAM")) {
			mem_edit.DrawContents(mInstance->mRAM.data(), mInstance->mRAM.size());
		}
	}

}