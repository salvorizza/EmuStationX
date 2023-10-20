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

		virtual void write(const std::string& busName, uint32_t address, uint8_t value) override;
		virtual void read(const std::string& busName, uint32_t address, uint8_t& output) override;

		virtual void write(const std::string& busName, uint32_t address, uint16_t value) override;
		virtual void read(const std::string& busName, uint32_t address, uint16_t& output) override;

		virtual void write(const std::string& busName, uint32_t address, uint32_t value) override;
		virtual void read(const std::string& busName, uint32_t address, uint32_t& output) override;

	private:
		std::vector<uint8_t> mMemory;
	};

}