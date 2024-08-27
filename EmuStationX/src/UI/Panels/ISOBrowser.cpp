#include "UI/Panels/ISOBrowser.h"

#include "UI/Window/FontAwesome5.h"
#include "UI/Panels/MemoryEditor.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

namespace esx {

	ISOBrowser::ISOBrowser()
		: Panel("ISO Browser", false)
	{}

	ISOBrowser::~ISOBrowser()
	{}


	void ISOBrowser::onImGuiRender()
	{

		static Vector<U8> selectedFileData;

		std::function<void(const String& ,const String&)> drawNode = [&](const String& parentPath, const String& path) {

			I32 charToRem = (parentPath != "\\" && parentPath != "") ? 1 : 0;
			I32 charToRemFile = (path != "\\" && path != "") ? 1 : 0;

			ImGui::PushID(path.c_str());

			String pathToShow = path;
			pathToShow.erase(0, parentPath.size() + charToRem);
			
			if (ImGui::TreeNodeEx(((path == "\\" ? String(ICON_FA_COMPACT_DISC) : String(ICON_FA_FOLDER)) + "  " + pathToShow).c_str())) {
				for (String& directoryPath : mISO9660->ListDirectories(path)) {
					drawNode(path, directoryPath);
				}

				for (String& filePath : mISO9660->ListFiles(path)) {
					pathToShow = filePath;
					pathToShow.erase(0, path.size() + charToRemFile);
					pathToShow.erase(pathToShow.size() - 2, 2);


					if (ImGui::TreeNodeEx((String(ICON_FA_FILE) + "  " + pathToShow).c_str(), ImGuiTreeNodeFlags_Leaf)) {
						ImGui::TreePop();
					}

					if (ImGui::IsItemActive()) {
						selectedFileData = mISO9660->GetFileData(filePath);
					}
				}

				ImGui::TreePop();
			}
			ImGui::PopID();
		};

		ImVec2 regionAvail = ImGui::GetContentRegionAvail();

		if (ImGui::BeginChild("Tree", ImVec2(regionAvail.x, regionAvail.y * 0.66f))) {
			drawNode("", "\\");
		}
		ImGui::EndChild();

		ImGui::Separator();

		static MemoryEditor mem_edit;
		mem_edit.DrawContents(selectedFileData.data(), selectedFileData.size());
			
	}

}