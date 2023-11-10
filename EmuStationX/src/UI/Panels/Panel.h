#pragma once

#include "UI/Window/ImGuiManager.h"

#include <string>

namespace esx {

	class Panel {
	public:
		Panel(const std::string& name, bool hasMenuBar, bool noPadding = false, bool defaultOpen = true, bool isModal = false);
		Panel(const std::string& name, bool hasMenuBar, ImVec4 bgColor, bool noPadding = false, bool defaultOpen = true, bool isModal = false);
		~Panel();

		void open();
		void render(const std::shared_ptr<ImGuiManager>& pManager);
		void close();

		inline bool isOpen() const { return mOpen; }
	protected:
		virtual void onOpen() {}
		virtual void onImGuiRender() = 0;
	protected:
		bool mOpen,mHasMenuBar,mNoPadding, mIsModal;
		std::string mName;
		std::shared_ptr<ImGuiManager> mManager;
		ImVec4 mBGColor;

	private:
		bool mFirstOpen;
	};

}
