#pragma once

#include <memory>

#include "Panel.h"
#include <Core/R3000.h>

namespace esx {

	class TTYPanel : public Panel {
	public:
		TTYPanel();
		~TTYPanel();

		void setInstance(const SharedPtr<R3000>& pInstance) { mInstance = pInstance; }

	protected:
		virtual void onImGuiRender() override;


	private:
		SharedPtr<R3000> mInstance;
	};

}