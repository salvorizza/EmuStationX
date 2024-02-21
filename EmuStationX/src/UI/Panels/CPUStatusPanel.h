#pragma once

#include <Core/R3000.h>

#include "Panel.h"

namespace esx {

	class CPUStatusPanel : public Panel {
	public:
		CPUStatusPanel();
		~CPUStatusPanel();

		void setInstance(const SharedPtr<R3000>& pInstance) { mInstance = pInstance; }

	protected:
		virtual void onImGuiRender() override;

	private:
		SharedPtr<R3000> mInstance;
	};

}