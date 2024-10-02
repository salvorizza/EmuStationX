#pragma once

#include <memory>

#include "Panel.h"
#include <Base/Bus.h>

#include "Core/ISO9660/ISO9660.h"

namespace esx {

	class ISOBrowser : public Panel {
	public:
		ISOBrowser();
		~ISOBrowser();

		void setInstance(const SharedPtr<ISO9660>& pInstance) { mISO9660 = pInstance; }

	protected:
		virtual void onImGuiRender() override;


	private:
		SharedPtr<ISO9660> mISO9660;
	};

}