#pragma once

#include <cstdint>

namespace esx {

	#define BYTE(x) x
	#define KIBI(x) x * BYTE(1024)
	#define MIBI(x) x * KIBI(1024)
}