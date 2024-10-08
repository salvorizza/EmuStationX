#include "UI/Panels/ControllerPanel.h"

#include "UI/Window/FontAwesome5.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

namespace esx {

	ControllerPanel::ControllerPanel()
		: Panel("Controller", false)
	{}

	ControllerPanel::~ControllerPanel()
	{}


	void ControllerPanel::onImGuiRender()
	{	
	}

}