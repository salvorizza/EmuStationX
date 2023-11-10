#include "UI/Panels/Panel.h"

#include <imgui.h>

namespace esx {

	Panel::Panel(const std::string& name, bool hasMenuBar, bool noPadding, bool defaultOpen, bool isModal)
		: mName(name),
		mOpen(defaultOpen),
		mHasMenuBar(hasMenuBar),
		mNoPadding(noPadding),
		mIsModal(isModal),
		mFirstOpen(false),
		mBGColor(ImGui::GetStyleColorVec4(ImGuiCol_WindowBg))
	{}


	Panel::Panel(const std::string& name, bool hasMenuBar, ImVec4 bgColor, bool noPadding, bool defaultOpen, bool isModal)
	:		mName(name),
			mOpen(defaultOpen),
			mHasMenuBar(hasMenuBar),
			mNoPadding(noPadding),
			mIsModal(isModal),
			mFirstOpen(false),
			mBGColor(bgColor)
	{}

	Panel::~Panel()
	{
	}

	void Panel::open()
	{
		onOpen();
		mFirstOpen = true;
		mOpen = true;
	}

	void Panel::render(const std::shared_ptr<ImGuiManager>& pManager)
	{
		mManager = pManager;
		if (mFirstOpen) {
			if (mIsModal) {
				ImGui::OpenPopup(mName.c_str());
			}
			mFirstOpen = false;
		}

		if (mOpen) {
			ImGuiWindowFlags window_flags = 0;
			if (mHasMenuBar) {
				window_flags |= ImGuiWindowFlags_MenuBar;
			}

			if (mNoPadding) {
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
			}
			if (mIsModal) {
				if (ImGui::BeginPopupModal(mName.c_str(), &mOpen)) {
					onImGuiRender();
					ImGui::EndPopup();
				}
			}
			else {
				ImGui::PushStyleColor(ImGuiCol_WindowBg, mBGColor);
				ImGui::Begin(mName.c_str(), &mOpen, window_flags);
				onImGuiRender();
				ImGui::End();
				ImGui::PopStyleColor(1);
				
			}

			if (mNoPadding) {
				ImGui::PopStyleVar(1);
			}
			
		}
	}

	void Panel::close()
	{
		mOpen = false;
	}

}