#pragma once

#include <memory>

#include "Panel.h"
#include <Base/Bus.h>

namespace esx {

	class KernelTables : public Panel {
	public:
		KernelTables();
		~KernelTables();

		void setInstance(const SharedPtr<Bus>& pInstance) { mBus = pInstance; }

	protected:
		virtual void onImGuiRender() override;


	private:
		SharedPtr<Bus> mBus;
	};

}