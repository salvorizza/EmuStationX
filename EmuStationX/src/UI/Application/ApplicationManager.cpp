#include "UI/Application/ApplicationManager.h"

namespace esx {
	ApplicationManager::ApplicationManager()
	{
	}

	ApplicationManager::~ApplicationManager()
	{
	}

	void ApplicationManager::run(const std::shared_ptr<Application>& pApplication)
	{
		mApplication = pApplication;
		mWindow = std::make_shared<Window>(mApplication->getName(), 1440, 900, pApplication->getICOPath());
		mImGuiManager = std::make_shared<ImGuiManager>(mWindow);

		mApplication->onSetup();

		mWindow->show();
		while (!mWindow->isClosed()) {
			mApplication->onUpdate();
			mApplication->onRender();

			mImGuiManager->startFrame();
			mApplication->onImGuiRender(mImGuiManager, mWindow);
			mImGuiManager->endFrame();

			mWindow->update();

			InputManager::Update();
		}

		mApplication->onCleanUp();
		mImGuiManager->cleanUp();
	}
}