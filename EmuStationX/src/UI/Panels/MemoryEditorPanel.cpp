#include "UI/Panels/MemoryEditorPanel.h"

#include <imgui.h>
#include <string>
#include "UI/Panels/MemoryEditor.h"

namespace esx {



	MemoryEditorPanel::MemoryEditorPanel()
		:	Panel("Memory Editor", false),
			mInstance(NULL)
	{}

	MemoryEditorPanel::~MemoryEditorPanel()
	{
	}


	void MemoryEditorPanel::onImGuiRender() {
		static MemoryEditor mem_edit;

		const char* items[] = { "RAM", "Bios"};
		static int item_current = 0;

		RAM* ram = mInstance->getDevice<RAM>(ESX_TEXT("RAM"));
		Bios* bios = mInstance->getDevice<Bios>(ESX_TEXT("Bios"));

		if (ImGui::BeginCombo("##combo", items[item_current])) // The second parameter is the label previewed before opening the combo.
		{
			for (int n = 0; n < IM_ARRAYSIZE(items); n++)
			{
				bool is_selected = (item_current == n); // You can store your selection however you want, outside or inside your objects

				ImGuiSelectableFlags flags = ImGuiSelectableFlags_None;
				switch (n) {
					case 0:
						flags |= ram->mMemory.size() == 0 ? ImGuiSelectableFlags_Disabled : 0;
						break;

					case 1:
						flags |= ram->mMemory.size() == 0 ? ImGuiSelectableFlags_Disabled : 0;
						break;
				}

				if (ImGui::Selectable(items[n], is_selected, flags))
					item_current = n;

				if (is_selected)
					ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
			}
			ImGui::EndCombo();
		}

		switch (item_current) {
			case 0:
				mem_edit.DrawContents(ram->mMemory.data(), ram->mMemory.size());
				break;

			case 1:
				mem_edit.DrawContents(bios->mMemory.data(), bios->mMemory.size());
				break;
		}
	}

}