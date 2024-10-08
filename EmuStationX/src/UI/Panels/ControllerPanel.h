#pragma once

#include "Panel.h"

namespace esx {

	class ControllerPanel : public Panel {
	public:
		ControllerPanel();
		~ControllerPanel();

	protected:
		virtual void onImGuiRender() override;
	};

}