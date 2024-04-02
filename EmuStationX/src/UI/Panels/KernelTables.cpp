#include "UI/Panels/KernelTables.h"


#include "Core/R3000.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>

namespace esx {

	static StringView fromStatusToString(U32 status) {
		switch (status) {
			case 0x0000: return ESX_TEXT("Free");
			case 0x2000: return ESX_TEXT("enabled/busy");
			case 0x4000: return ESX_TEXT("enabled/ready");
		}
		return ESX_TEXT("None");
	}

	static StringView fromClassToString(U32 entryClass) {
		if (entryClass >> 24 == 0xF1) {
			return ESX_TEXT("Event");
		}

		if (entryClass >> 24 == 0xF3) {
			return ESX_TEXT("User");
		}

		if (entryClass >> 24 == 0xFF) {
			return ESX_TEXT("Thread");
		}

		switch (entryClass) {
			case 0x00000000: 
			case 0x00000001:
			case 0x00000002:
			case 0x00000003:
			case 0x00000004:
			case 0x00000005:
			case 0x00000006:
			case 0x00000007:
			case 0x00000008:
			case 0x00000009:
			case 0x0000000A:
			case 0x0000000B:
			case 0x0000000C:
			case 0x0000000D:
			case 0x0000000E:
			case 0x0000000F:
				return ESX_TEXT("Memory card");

			case 0xF0000001: return ESX_TEXT("VBLANK");
			case 0xF0000002: return ESX_TEXT("GPU");
			case 0xF0000003: return ESX_TEXT("CDROM");
			case 0xF0000004: return ESX_TEXT("DMA");
			case 0xF0000005: return ESX_TEXT("RTC0");
			case 0xF0000006: return ESX_TEXT("RTC1");
			case 0xF0000007: return ESX_TEXT("");
			case 0xF0000008: return ESX_TEXT("Controller");
			case 0xF0000009: return ESX_TEXT("SPU");
			case 0xF000000A: return ESX_TEXT("PIO");
			case 0xF000000B: return ESX_TEXT("SIO");
			case 0xF0000010: return ESX_TEXT("Exception");
			case 0xF0000011: return ESX_TEXT("MemoryCard");
			case 0xF0000012: return ESX_TEXT("MemoryCard");
			case 0xF0000013: return ESX_TEXT("MemoryCard");

			case 0xF2000000: return ESX_TEXT("Root counter 0");
			case 0xF2000001: return ESX_TEXT("Root counter 1");
			case 0xF2000002: return ESX_TEXT("Root counter 2");
			case 0xF2000003: return ESX_TEXT("Root counter 3");

			case 0xF4000001: return ESX_TEXT("MemoryCard");
			case 0xF4000002: return ESX_TEXT("LibMath");
		}

		return ESX_TEXT("None");
	}

	static StringView fromSpecToString(U32 spec) {
		switch (spec) {
			case 0x0001: return ESX_TEXT("Counter becomes zero");
			case 0x0002: return ESX_TEXT("interrupted");
			case 0x0004: return ESX_TEXT("End of IO");
			case 0x0008: return ESX_TEXT("file was closed");
			case 0x0010: return ESX_TEXT("command acknowledged");
			case 0x0020: return ESX_TEXT("command completed");
			case 0x0040: return ESX_TEXT("data ready");
			case 0x0080: return ESX_TEXT("data end");
			case 0x0100: return ESX_TEXT("time out");
			case 0x0200: return ESX_TEXT("unknown command");
			case 0x0400: return ESX_TEXT("end of read buffer");
			case 0x0800: return ESX_TEXT("end of write buffer");
			case 0x1000: return ESX_TEXT("general interrupt");
			case 0x2000: return ESX_TEXT("new device");
			case 0x4000: return ESX_TEXT("system call instruction; SYS(04h..FFFFFFFFh)");
			case 0x8000: return ESX_TEXT("error happened");
			case 0x8001: return ESX_TEXT("previous write error happened");
			case 0x0301: return ESX_TEXT("domain error in libmath");
			case 0x0302: return ESX_TEXT("range error in libmath");
		}
		return ESX_TEXT("None");
	}
	
	static StringView fromModeToString(U32 mode) {
		switch (mode) {
			case 0x1000: return ESX_TEXT("execute function/stay busy");
			case 0x2000: return ESX_TEXT("no func/mark ready");
		}
		return ESX_TEXT("None");
	}

	KernelTables::KernelTables()
		: Panel("Kernel Tables", false, true)
	{}

	KernelTables::~KernelTables()
	{}


	void KernelTables::onImGuiRender()
	{
		U32 tableStartAddress = R3000::toPhysicalAddress(mBus->load<U32>(0x120));
		U32 tableSize = mBus->load<U32>(0x120 + 4);
		U32 entrySize = 0x1C;

		if (tableStartAddress != 0) {
			if (ImGui::BeginTable("KernelTable", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
				ImGui::TableSetupColumn("Class");
				ImGui::TableSetupColumn("Status");
				ImGui::TableSetupColumn("Spec");
				ImGui::TableSetupColumn("Mode");
				ImGui::TableSetupColumn("Handler");
				ImGui::TableHeadersRow();

				for (U32 entryAddress = tableStartAddress; entryAddress < tableStartAddress + tableSize; entryAddress += entrySize) {
					U32 entryClass = mBus->load<U32>(entryAddress + 0x00);
					U32 entryStatus = mBus->load<U32>(entryAddress + 0x04);
					U32 entrySpec = mBus->load<U32>(entryAddress + 0x08);
					U32 entryMode = mBus->load<U32>(entryAddress + 0x0C);
					U32 entryHandler = mBus->load<U32>(entryAddress + 0x10);

					StringView classString = fromClassToString(entryClass);
					StringView specString = fromSpecToString(entrySpec);
					StringView statusString = fromStatusToString(entryStatus);
					StringView modeString = fromModeToString(entryMode);

					ImGui::TableNextColumn();
					ImGui::Text(classString.data());

					ImGui::TableNextColumn();
					ImGui::Text("0x%04X: %s", entryStatus, statusString.data());

					ImGui::TableNextColumn();
					ImGui::Text("0x%04X: %s", entrySpec, specString.data());

					ImGui::TableNextColumn();
					ImGui::Text(modeString.data());

					ImGui::TableNextColumn();
					ImGui::Text("0x%08X", entryHandler);


					ImGui::TableNextRow();
				}
				ImGui::EndTable();
			}
		}

	}

}