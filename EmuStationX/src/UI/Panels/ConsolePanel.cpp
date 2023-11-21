#include "UI/Panels/ConsolePanel.h"	


namespace esx {



	ConsolePanel::ConsolePanel()
		:	Panel("Console", true)
	{}

	ConsolePanel::~ConsolePanel()
	{
	}


	void ConsolePanel::onImGuiRender() {
		mConsole.Draw();
	}

}