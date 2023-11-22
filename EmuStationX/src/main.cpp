#include "UI/Application/ApplicationManager.h"

#include "UI/Panels/ViewportPanel.h"
#include "UI/Panels/CPUStatusPanel.h"
#include "UI/Panels/MemoryEditorPanel.h"
#include "UI/Panels/DisassemblerPanel.h"
#include "UI/Panels/ConsolePanel.h"

#include "UI/Graphics/BatchRenderer.h"
#include "UI/Window/FontAwesome5.h"
#include "UI/Utils.h"

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

#ifdef ESX_PLATFORM_WINDOWS
	#include <Windows.h>
#endif // ESX_PLATFORM_WINDOWS

#undef ERROR

using namespace esx;

class EmuStationXLogger : public Logger {
public:
	EmuStationXLogger(const std::shared_ptr<ConsolePanel>& consolePanel)
		: Logger(ESX_TEXT("Core")),
			mConsolePanel(consolePanel)
	{}

	~EmuStationXLogger() = default;

	virtual void Log(LogType type, const StringView& message) override {
		if (mConsolePanel) {
			switch (type)
			{
				case esx::LogType::Info:
					mConsolePanel->getInternalConsole().System().Log(csys::ItemType::INFO) << message;
					break;
				case esx::LogType::Trace:
					mConsolePanel->getInternalConsole().System().Log(csys::ItemType::LOG) << message;
					break;
				case esx::LogType::Warning:
					mConsolePanel->getInternalConsole().System().Log(csys::ItemType::WARNING) << message;
					break;
				case esx::LogType::Error:
					mConsolePanel->getInternalConsole().System().Log(csys::ItemType::ERROR) << message;
					break;
				case esx::LogType::Fatal:
					mConsolePanel->getInternalConsole().System().Log(csys::ItemType::ERROR) << message;
					break;
				default:
					break;
			}
		}
	}
private:
	std::shared_ptr<ConsolePanel> mConsolePanel;
};

class EmuStationXApp : public Application {
public:
	EmuStationXApp()
		: Application("EmuStationX", "commons/icons/Logo.ico"),
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
		mConsolePanel = std::make_shared<ConsolePanel>();
		mLogger = std::make_shared<EmuStationXLogger>(mConsolePanel);

		LoggingSystem::SetCoreLogger(mLogger);

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
				if (ImGui::MenuItem("Console", "CTRL+O")) mConsolePanel->open();

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
		mConsolePanel->render(pManager);
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
	std::shared_ptr<ConsolePanel> mConsolePanel;
	std::shared_ptr<EmuStationXLogger> mLogger;

};

int
#if !defined(_MAC)
	#if defined(_M_CEE_PURE)
		__clrcall
	#else
		WINAPI
	#endif
#else
	CALLBACK
#endif
WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nShowCmd
) {
	LoggingSpecifications specs(ESX_TEXT(""));
	LoggingSystem::Start(specs);

	ApplicationManager appManager;
	appManager.run(std::make_shared<EmuStationXApp>());

	LoggingSystem::Shutdown();
	return 0;
};