#include "UI/Application/ApplicationManager.h"

#include "UI/Panels/ViewportPanel.h"
#include "UI/Panels/CPUStatusPanel.h"
#include "UI/Panels/MemoryEditorPanel.h"
#include "UI/Panels/DisassemblerPanel.h"
#include "UI/Panels/ConsolePanel.h"
#include "UI/Panels/KernelTables.h"
#include "UI/Panels/SPUStatusPanel.h"
#include "UI/Panels/TTYPanel.h"
#include "UI/Panels/FileDialogPanel.h"
#include "UI/Panels/ISOBrowser.h"

#include "UI/Graphics/BatchRenderer.h"
#include "UI/Window/FontAwesome5.h"
#include "UI/Window/ControllerManager.h"
#include "UI/Utils.h"
#include "UI/Application/LoopTimer.h"

#include "Utils/LoggingSystem.h"

#include "Base/Base.h"
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
#include "Core/CDROM.h"
#include "Core/SIO.h"
#include "Core/Controller.h"
#include "Core/MemoryCard.h"
#include "Core/MDEC.h"


#ifdef ESX_PLATFORM_WINDOWS
	#include <Windows.h>
	#undef ERROR
#endif // ESX_PLATFORM_WINDOWS

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

#include <iostream>

#include "optick.h"
#include "miniaudio.h"

#include "Core/CD/CDRWIN.h"
#include "Core/CD/ISO.h"

#include "Core/ISO9660/ISO9660.h"

#include "Platform/Win32/CDROMDrive.h"
#include "Core/CD/CDROMDisk.h"



using namespace esx;
#undef max;
#undef min;

struct FPSCounter {
	LoopTimer Timer = {};
	I32 MaxFPS = 0;
	I32 MinFPS = 0;
	I32 CurrentFPS = 0;
	U64 NumSamples = 0;
	U64 SumFPS = 0;

	void Init() {
		Timer.init();
		MaxFPS = 0;
		MinFPS = 0;
		CurrentFPS = 0;
		NumSamples = 0;
		SumFPS = 0;
	}

	void Update() {
		Timer.update();
		NumSamples++;

		CurrentFPS = (I32)(1.0 / Timer.getDeltaTimeInSeconds());
		MaxFPS = std::max<I32>(MaxFPS, CurrentFPS);
		MinFPS = std::min<I32>(MinFPS, CurrentFPS);
		SumFPS += CurrentFPS;
	}

	I32 AvgFPS() {
		if (NumSamples == 0) return 0;
		return SumFPS / NumSamples;
	}
};

class EmuStationXLogger : public Logger {
public:
	EmuStationXLogger(const SharedPtr<ConsolePanel>& consolePanel)
		: Logger(ESX_TEXT("Core")),
			mConsolePanel(consolePanel)
	{}

	~EmuStationXLogger() = default;

	virtual void Log(LogType type, const StringView& message) override {
		if ((I32)type < mLogLevel) return;

		auto& items = mConsolePanel->getInternalConsole().System().Items();
		if (items.size() > 1000) {
			//items.pop_front();
		}

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

	void SetLogLevel(LogType logLevel) {
		mLogLevel = (I32)logLevel;
	}
private:
	SharedPtr<ConsolePanel> mConsolePanel;
	I32 mLogLevel = -1;
};

class EmuStationXApp : public Application {
public:
	EmuStationXApp()
		: Application("EmuStationX", "commons/icons/Logo.ico")
	{
	}

	~EmuStationXApp() {
	}

	virtual void onSetup() override {
		mConsolePanel = MakeShared<ConsolePanel>();
		mLogger = MakeShared<EmuStationXLogger>(mConsolePanel);
		LoggingSystem::SetCoreLogger(mLogger);

		//mLogger->SetLogLevel(LogType::Error);

		mCPUStatusPanel = MakeShared<CPUStatusPanel>();
		mDisassemblerPanel = MakeShared<DisassemblerPanel>();
		mMemoryEditorPanel = MakeShared<MemoryEditorPanel>();
		mBatchRenderer = MakeShared<BatchRenderer>();
		mViewportPanel = MakeShared<ViewportPanel>();
		mKernelTablesPanel = MakeShared<KernelTables>();
		mSPUStatusPanel = MakeShared<SPUStatusPanel>();
		mTTYPanel = MakeShared<TTYPanel>();
		mFileDialogPanel = MakeShared<FileDialogPanel>();
		mISOBrowser = MakeShared<ISOBrowser>();

		mFileDialogPanel->setCurrentPath("commons/games");
		mFileDialogPanel->setOnFileSelectedCallback(std::bind(&EmuStationXApp::onFileSelected, this, std::placeholders::_1));

		root = MakeShared<Bus>(ESX_TEXT("Root"));
		cpu = MakeShared<R3000>();
		mainRAM = MakeShared<RAM>("RAM", 0x00000000, MIBI(8), MIBI(2));
		scratchPad = MakeShared<RAM>("Scratchpad", 0x1F800000, 0x400, KIBI(1), ESX_FALSE);
		memoryControl = MakeShared<MemoryControl>();
		interruptControl = MakeShared<InterruptControl>();
		spu = MakeShared<SPU>();
		pio = MakeShared<PIO>();
		bios = MakeShared<Bios>("commons/bios/scph1001.bin");
		timer = MakeShared<Timer>();
		dma = MakeShared<DMA>();
		gpu = MakeShared<GPU>(mBatchRenderer);
		cdrom = MakeShared<CDROM>();
		sio0 = MakeShared<SIO>(0);
		sio1 = MakeShared<SIO>(1);
		controller = MakeShared<Controller>(ControllerType::DigitalPad);
		memoryCard = MakeShared<MemoryCard>(0);
		memoryCard2 = MakeShared<MemoryCard>(1);
		mdec = MakeShared<MDEC>();

		memoryCard->LoadFromFile("commons/memory_cards/0.mcr");
		memoryCard2->LoadFromFile("commons/memory_cards/1.mcr");

		root->connectDevice(cpu);
		cpu->connectToBus(root);

		root->connectDevice(bios);
		bios->connectToBus(root);

		root->connectDevice(mainRAM);
		mainRAM->connectToBus(root);

		root->connectDevice(scratchPad);
		scratchPad->connectToBus(root);

		root->connectDevice(memoryControl);
		memoryControl->connectToBus(root);

		root->connectDevice(spu);
		spu->connectToBus(root);

		root->connectDevice(pio);
		pio->connectToBus(root);

		root->connectDevice(interruptControl);
		interruptControl->connectToBus(root);

		root->connectDevice(timer);
		timer->connectToBus(root);

		root->connectDevice(dma);
		dma->connectToBus(root);

		root->connectDevice(gpu);
		gpu->connectToBus(root);

		root->connectDevice(cdrom);
		cdrom->connectToBus(root);

		root->connectDevice(sio0);
		sio0->connectToBus(root);

		root->connectDevice(sio1);
		sio1->connectToBus(root);

		root->connectDevice(mdec);
		mdec->connectToBus(root);

		sio0->plugDevice(SerialPort::Port1, controller);
		controller->setMaster(sio0);

		sio0->plugDevice(SerialPort::Port1, memoryCard);
		memoryCard->setMaster(sio0);

		/*sio0->plugDevice(SerialPort::Port2, memoryCard2);
		memoryCard2->setMaster(sio0);*/

		root->sortRanges();

		mCPUStatusPanel->setInstance(cpu);
		mDisassemblerPanel->setInstance(cpu);
		mDisassemblerPanel->setGPU(gpu);
		mDisassemblerPanel->setBus(root);
		mMemoryEditorPanel->setInstance(root);
		mKernelTablesPanel->setInstance(root);
		mSPUStatusPanel->setInstance(spu);
		mTTYPanel->setInstance(cpu);

		mBatchRenderer->Begin();

		ma_device_config config = ma_device_config_init(ma_device_type_playback);
		config.playback.format = ma_format_s16;
		config.playback.channels = 2;
		config.sampleRate = 44100;
		config.dataCallback = audioCallback;
		config.pUserData = this;

		if (ma_device_init(NULL, &config, &mAudioDevice) != MA_SUCCESS) {
			ESX_CORE_LOG_ERROR("failed to init MiniAudio");
		}

		ma_device_start(&mAudioDevice);
		mNumPrerendered = PRERENDERED_SIZE;

		InputManager::Init();
		fpsCounter.Init();

		Vector<String> cdroms = platform::CDROMDrive::List();
		if (cdroms.size() > 0) {
			mCDROMDrive = MakeShared<platform::CDROMDrive>(cdroms[0]);
			auto cd = MakeShared<CDROMDisk>(mCDROMDrive);
			cdrom->insertCD(cd);
			mISO9660 = MakeShared<ISO9660>(cd);
			mISOBrowser->setInstance(mISO9660);
			mCurrentGame = getBootNameFromSystemConfig();
			hardReset();
		}
	}

	using HandlerFunction = std::function<SharedPtr<CompactDisk>(const std::filesystem::path&)>;

	String getBootNameFromSystemConfig() {
		const DirectoryRecord& info = mISO9660->GetFileInfo("\\SYSTEM.CNF;1");
		Vector<U8> cnfData = Vector<U8>(info.DataSizeLE);
		mISO9660->GetFileData("\\SYSTEM.CNF;1", cnfData);
		String systemCnf = String(cnfData.begin(), cnfData.end());
		size_t end = systemCnf.find(String(";1"));
		size_t start = systemCnf.find(String(":\\")) + 2;
		return systemCnf.substr(start, (end - start));
	}

	void onFileSelected(const std::filesystem::path& filePath) {
		static UnorderedMap<String, HandlerFunction> handlers = {
		   { ".exe", [&](const std::filesystem::path& filePath) {
			   mDisassemblerPanel->loadEXE(filePath);
			   return nullptr;
		   }},
		   { ".ps-exe", [&](const std::filesystem::path& filePath) {
			   mDisassemblerPanel->loadEXE(filePath);
			   return nullptr;
		   }},
		   { ".cue", [&](const std::filesystem::path& filePath) {
			   return MakeShared<CDRWIN>(filePath);
		   }},
		   { ".iso", [&](const std::filesystem::path& filePath) {
			   return MakeShared<ISO>(filePath);
		   }}
		};

		const auto& extension = filePath.extension().string();
		if (handlers.contains(extension)) {
			auto cd = handlers[extension](filePath);
			if (cd) {
				cdrom->insertCD(cd);
				mISO9660 = MakeShared<ISO9660>(cd);
				mISOBrowser->setInstance(mISO9660);
				mCurrentGame = getBootNameFromSystemConfig();
			}
			hardReset();
			ESX_CORE_LOG_INFO("File {} loaded", filePath.stem().string());
		} else {
			ESX_CORE_LOG_ERROR("File not supported yet");
		}
	}

	static void audioCallback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
	{
		if (frameCount == 0) {
			return;
		}

		EmuStationXApp* pApp = (EmuStationXApp*)pDevice->pUserData;
		auto spu = pApp->spu;

		std::scoped_lock<std::mutex> lc(spu->mSamplesMutex);
		if (spu->mFramesQueue.empty()) {
			return;
		}

		auto& batch = spu->mFramesQueue.front();
		if (pApp->mNumPrerendered == pApp->PRERENDERED_SIZE && (spu->mFramesQueue.size() - 1) >= pApp->PRERENDERED_SIZE) {
			pApp->mNumPrerendered = 0;
		}

		if (pApp->mNumPrerendered > 0 && batch.Complete()) {
			std::memcpy(pOutput, batch.Batch.data(), sizeof(AudioFrame) * frameCount);
			spu->mFramesQueue.pop_front();
			pApp->mNumPrerendered++;
		}
	}

	virtual void onUpdate() override {
		OPTICK_FRAME("MainThread");

#if 0
		controller->setButtonState(ControllerButton::JoypadDown, ControllerManager::IsButtonPressed(0, GLFW_GAMEPAD_BUTTON_DPAD_DOWN));
		controller->setButtonState(ControllerButton::JoypadUp, ControllerManager::IsButtonPressed(0, GLFW_GAMEPAD_BUTTON_DPAD_UP));
		controller->setButtonState(ControllerButton::JoypadLeft, ControllerManager::IsButtonPressed(0, GLFW_GAMEPAD_BUTTON_DPAD_LEFT));
		controller->setButtonState(ControllerButton::JoypadRight, ControllerManager::IsButtonPressed(0, GLFW_GAMEPAD_BUTTON_DPAD_RIGHT));

		controller->setButtonState(ControllerButton::Cross, ControllerManager::IsButtonPressed(0, GLFW_GAMEPAD_BUTTON_CROSS));
		controller->setButtonState(ControllerButton::Square, ControllerManager::IsButtonPressed(0, GLFW_GAMEPAD_BUTTON_SQUARE));
		controller->setButtonState(ControllerButton::Triangle, ControllerManager::IsButtonPressed(0, GLFW_GAMEPAD_BUTTON_TRIANGLE));
		controller->setButtonState(ControllerButton::Circle, ControllerManager::IsButtonPressed(0, GLFW_GAMEPAD_BUTTON_CIRCLE));

		controller->setButtonState(ControllerButton::Select, ControllerManager::IsButtonPressed(0, GLFW_GAMEPAD_BUTTON_BACK));
		controller->setButtonState(ControllerButton::Start, ControllerManager::IsButtonPressed(0, GLFW_GAMEPAD_BUTTON_START));

		controller->setButtonState(ControllerButton::R1, ControllerManager::IsButtonPressed(0, GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER));
		controller->setButtonState(ControllerButton::L1, ControllerManager::IsButtonPressed(0, GLFW_GAMEPAD_BUTTON_LEFT_BUMPER));

		controller->setButtonState(ControllerButton::R2, ControllerManager::IsButtonPressed(0, GLFW_GAMEPAD_BUTTON_RIGHT_THUMB));
		controller->setButtonState(ControllerButton::L2, ControllerManager::IsButtonPressed(0, GLFW_GAMEPAD_BUTTON_LEFT_THUMB));
#else
		controller->setButtonState(ControllerButton::JoypadDown, InputManager::IsKeyPressed(GLFW_KEY_S));
		controller->setButtonState(ControllerButton::JoypadUp, InputManager::IsKeyPressed(GLFW_KEY_W));
		controller->setButtonState(ControllerButton::JoypadLeft, InputManager::IsKeyPressed(GLFW_KEY_A));
		controller->setButtonState(ControllerButton::JoypadRight, InputManager::IsKeyPressed(GLFW_KEY_D));

		controller->setButtonState(ControllerButton::Cross, InputManager::IsKeyPressed(GLFW_KEY_Z));
		controller->setButtonState(ControllerButton::Square, InputManager::IsKeyPressed(GLFW_KEY_X));
		controller->setButtonState(ControllerButton::Triangle, InputManager::IsKeyPressed(GLFW_KEY_C));
		controller->setButtonState(ControllerButton::Circle, InputManager::IsKeyPressed(GLFW_KEY_V));

		controller->setButtonState(ControllerButton::Select, InputManager::IsKeyPressed(GLFW_KEY_K));
		controller->setButtonState(ControllerButton::Start, InputManager::IsKeyPressed(GLFW_KEY_L));

		controller->setButtonState(ControllerButton::R1, InputManager::IsKeyPressed(GLFW_KEY_P));
		controller->setButtonState(ControllerButton::L1, InputManager::IsKeyPressed(GLFW_KEY_Q));
#endif

		mDisassemblerPanel->onUpdate();
		mViewportPanel->setFrame(mBatchRenderer->getPreviousFrame());

		mViewportPanel->setFrame(mBatchRenderer->getPreviousFrame());
		fpsCounter.Update();
	}

	virtual void onRender() override {
	}

	virtual void onCleanUp() override {
		ma_device_uninit(&mAudioDevice);
	}

	virtual void onImGuiRender(const SharedPtr<ImGuiManager>& pManager, const SharedPtr<Window>& pWindow) override {
		static bool p_open = true;

		mFileDialogPanel->setIconForExtension(".*", pManager->LoadIconResource("commons/icons/file.png"), "FILE");
		mFileDialogPanel->setFolderIcon(pManager->LoadIconResource("commons/icons/folder.png"));

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
				if (ImGui::MenuItem("Play")) mDisassemblerPanel->onPlay();
				if (ImGui::MenuItem("Pause")) mDisassemblerPanel->onPause();
				if (ImGui::MenuItem("Hard Reset")) hardReset();

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Memory Card"))
			{
				if (ImGui::MenuItem("Save Memory Card 1")) memoryCard->Save();
				if (ImGui::MenuItem("Save Memory Card 2")) memoryCard2->Save();

				ImGui::EndMenu();
			}

			ImGui::Text("Min FPS: %d", fpsCounter.MinFPS);
			ImGui::Text("Avg FPS: %d", fpsCounter.AvgFPS());
			ImGui::Text("Max FPS: %d", fpsCounter.MaxFPS);
			ImGui::Text("Current FPS: %d", fpsCounter.CurrentFPS);

			ImGui::TextUnformatted(mCurrentGame.c_str());

			ImGui::EndMenuBar();
		}


		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

		ImGui::End();

		mCPUStatusPanel->render(pManager);
		mMemoryEditorPanel->render(pManager);
		mDisassemblerPanel->render(pManager);
		mConsolePanel->render(pManager);
		mViewportPanel->render(pManager);
		mKernelTablesPanel->render(pManager);
		mSPUStatusPanel->render(pManager);
		mTTYPanel->render(pManager);
		mFileDialogPanel->render(pManager);
		mISOBrowser->render(pManager);
	}

	void hardReset() {
		fpsCounter.Init();
		cpu->reset();
		mainRAM->reset();
		scratchPad->reset();
		memoryControl->reset();
		interruptControl->reset();
		spu->reset();
		pio->reset();
		bios->reset();
		timer->reset();
		dma->reset();
		gpu->reset();
		cdrom->reset();
		sio0->reset();
		sio1->reset();
		mdec->reset();
		mConsolePanel->getInternalConsole().System().Items().clear();
		if (mDisassemblerPanel->getDebugState() == DebugState::Breakpoint) {
			mDisassemblerPanel->onPlay();
		}
	}

private:
	SharedPtr<Bus> root;
	SharedPtr<R3000> cpu;
	SharedPtr<RAM> mainRAM;
	SharedPtr<RAM> scratchPad;
	SharedPtr<MemoryControl> memoryControl;
	SharedPtr<InterruptControl> interruptControl;
	SharedPtr<SPU> spu;
	SharedPtr<PIO> pio;
	SharedPtr<Bios> bios;
	SharedPtr<Timer> timer;
	SharedPtr<DMA> dma;
	SharedPtr<GPU> gpu;
	SharedPtr<CDROM> cdrom;
	SharedPtr<SIO> sio0;
	SharedPtr<SIO> sio1;
	SharedPtr<Controller> controller;
	SharedPtr<MemoryCard> memoryCard, memoryCard2;
	SharedPtr<MDEC> mdec;
	FPSCounter fpsCounter;


	SharedPtr<CPUStatusPanel> mCPUStatusPanel;
	SharedPtr<DisassemblerPanel> mDisassemblerPanel;
	SharedPtr<MemoryEditorPanel> mMemoryEditorPanel;
	SharedPtr<ConsolePanel> mConsolePanel;
	SharedPtr<EmuStationXLogger> mLogger;
	SharedPtr<BatchRenderer> mBatchRenderer;
	SharedPtr<ViewportPanel> mViewportPanel;
	SharedPtr<KernelTables> mKernelTablesPanel;
	SharedPtr<SPUStatusPanel> mSPUStatusPanel;
	SharedPtr<TTYPanel> mTTYPanel;
	SharedPtr<FileDialogPanel> mFileDialogPanel;
	SharedPtr<ISOBrowser> mISOBrowser;
	glm::mat4 mProjectionMatrix;

	ma_device mAudioDevice;
	const U32 PRERENDERED_SIZE = 64;
	U32 mNumPrerendered = 0;

	SharedPtr<platform::CDROMDrive> mCDROMDrive;
	SharedPtr<ISO9660> mISO9660;
	String mCurrentGame = "";
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
	appManager.run(MakeShared<EmuStationXApp>());

	LoggingSystem::Shutdown();
	return 0;
};