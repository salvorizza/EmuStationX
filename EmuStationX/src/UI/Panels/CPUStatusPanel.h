#pragma once

#include <Core/R3000.h>

#include "Panel.h"

namespace esx {

	class CPUStatusPanel : public Panel {
	public:
		CPUStatusPanel();
		~CPUStatusPanel();

		void setInstance(R3000* pInstance) { mInstance = pInstance; }

	protected:
		virtual void onImGuiRender() override;

	private:
		R3000* mInstance;
	};

}