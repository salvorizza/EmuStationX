#pragma once

#include <Base/Bus.h>
#include <Core/RAM.h>
#include <Core/Bios.h>

#include "Panel.h"

namespace esx {

	class MemoryEditorPanel : public Panel {
	public:
		MemoryEditorPanel();
		~MemoryEditorPanel();

		void setInstance(Bus* pInstance) { mInstance = pInstance; }

	protected:
		virtual void onImGuiRender() override;

	private:
		Bus* mInstance;
	};

}