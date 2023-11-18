#include "UI/Application/ApplicationManager.h"

#include "UI/Panels/ViewportPanel.h"
#include "UI/Panels/CPUStatusPanel.h"
#include "UI/Panels/MemoryEditorPanel.h"
#include "UI/Panels/DisassemblerPanel.h"

#include "UI/Graphics/BatchRenderer.h"
#include "UI/Window/FontAwesome5.h"

#include "Utils/LoggingSystem.h"

#include "Base/Bus.h"
#include "Core/R3000.h"
#include "Core/Bios.h"
#include "Core/RAM.h"
#include "Core/MemoryControl.h"
#include "Core/SPU.h"
#include "Core/PIO.h"
#include "Core/InterruptControl.h"
#include "Core/Timer.h"
#include "Core/DMA.h"
#include "Core/GPU.h"


#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

#include <iostream>

using namespace esx;


class EmuStationXApp : public Application {
public:
	EmuStationXApp()
		: Application("EmuStationX"),
			bios(ESX_TEXT("scph1001.bin")),
			root(ESX_TEXT("Root"))
	{
	}

	~EmuStationXApp() {
	}

	virtual void onSetup() override {
		mCPUStatusPanel = std::make_shared<CPUStatusPanel>();
		mDisassemblerPanel = std::make_shared<DisassemblerPanel>();
		mMemoryEditorPanel = std::make_shared<MemoryEditorPanel>();

		root.connectDevice(&cpu);
		root.connectDevice(&bios);
		root.connectDevice(&mainRAM);
		root.connectDevice(&memoryControl);
		root.connectDevice(&spu);
		root.connectDevice(&pio);
		root.connectDevice(&interruptControl);
		root.connectDevice(&timer);
		root.connectDevice(&dma);
		root.connectDevice(&gpu);

		mCPUStatusPanel->setInstance(&cpu);
		mDisassemblerPanel->setInstance(&cpu);
		mMemoryEditorPanel->setInstance(&root);
	}

	virtual void onUpdate() override {
		mDisassemblerPanel->onUpdate();
	}

	virtual void onRender() override {
	}

	virtual void onImGuiRender(const std::shared_ptr<ImGuiManager>& pManager, const std::shared_ptr<Window>& pWindow) override {
		static bool p_open = true;

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_MenuBar;
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		window_flags |= ImGuiWindowFlags_NoTitleBar;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		ImGui::Begin("ReviveNES", &p_open, window_flags);
		ImGui::PopStyleVar();

		if (ImGui::BeginMenuBar())
		{

			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Open", "CTRL+M")) {
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Tools"))
			{
				if (ImGui::MenuItem("CPU Status", "CTRL+R")) mCPUStatusPanel->open();
				if (ImGui::MenuItem("Debugger", "CTRL+D")) mDisassemblerPanel->open();
				if (ImGui::MenuItem("Memory", "CTRL+M")) mMemoryEditorPanel->open();

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Emulation"))
			{
				ImGui::MenuItem("Play");
				ImGui::MenuItem("Stop");
				ImGui::MenuItem("Pause");

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}


		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

		ImGui::End();

		mCPUStatusPanel->render(pManager);
		mMemoryEditorPanel->render(pManager);
		mDisassemblerPanel->render(pManager);
	}

private:
	Bus root;
	R3000 cpu;
	RAM mainRAM;
	MemoryControl memoryControl;
	InterruptControl interruptControl;
	SPU spu;
	PIO pio;
	Bios bios;
	Timer timer;
	DMA dma;
	GPU gpu;

	std::shared_ptr<CPUStatusPanel> mCPUStatusPanel;
	std::shared_ptr<DisassemblerPanel> mDisassemblerPanel;
	std::shared_ptr<MemoryEditorPanel> mMemoryEditorPanel;

};

int main(int argc, char** argv) {
	LoggingSpecifications specs(ESX_TEXT(""));
	LoggingSystem::Start(specs);

	ApplicationManager appManager;
	appManager.run(std::make_shared<EmuStationXApp>());

	LoggingSystem::Shutdown();
	return 0;
}