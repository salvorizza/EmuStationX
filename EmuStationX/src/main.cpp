#include <iostream>

#include "Base/Bus.h"
#include "Core/R3000.h"
#include "Core/Bios.h"
#include "Core/RAM.h"

using namespace esx;

int main(int argc, char** argv) {
	R3000 cpu;
	Bios bios("scph1001.bin");
	RAM mainRAM;

	Bus root("Root");

	root.connectDevice(&cpu);
	root.connectDevice(&bios);
	root.connectDevice(&mainRAM);

	while (true) {
		cpu.clock();
	}

	return 0;
}