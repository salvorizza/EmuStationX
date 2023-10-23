#pragma once

#include <string_view>
#include <vector>
#include <fstream>

#include "Base/Base.h"
#include "Base/Bus.h"

namespace esx {


	class Bios : public BusDevice {
	public:
		Bios(const std::string& path);
		~Bios();

		virtual void write(const std::string& busName, uint32_t address, uint32_t value, size_t valueSize) override;
		virtual uint32_t read(const std::string& busName, uint32_t address, size_t outputSize) override;

	private:
		std::vector<uint8_t> mMemory;
	};

}