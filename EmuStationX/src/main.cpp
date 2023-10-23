#include <iostream>

#include "Base/Bus.h"
#include "Core/R3000.h"
#include "Core/Bios.h"
#include "Core/RAM.h"
#include "Core/MemoryControl.h"
#include "Core/SPU.h"

using namespace esx;

int main(int argc, char** argv) {
	R3000 cpu;
	RAM mainRAM;
	MemoryControl memoryControl;
	SPU spu;
	Bios bios("scph1001.bin");

	Bus root("Root");

	root.connectDevice(&cpu);
	root.connectDevice(&bios);
	root.connectDevice(&mainRAM);
	root.connectDevice(&memoryControl);
	root.connectDevice(&spu);

	while (true) {
		cpu.clock();
	}

	return 0;
}