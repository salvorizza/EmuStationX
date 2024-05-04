#pragma once

#include <Core/SPU.h>

#include "Panel.h"

namespace esx {

	class SPUStatusPanel : public Panel {
	public:
		SPUStatusPanel();
		~SPUStatusPanel();

		void setInstance(const SharedPtr<SPU>& pInstance) { mInstance = pInstance; }

	protected:
		virtual void onImGuiRender() override;

	private:
		SharedPtr<SPU> mInstance;
	};

}