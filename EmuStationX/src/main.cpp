#include <iostream>

#include "Utils/LoggingSystem.h"

#include "Base/Bus.h"
#include "Core/R3000.h"
#include "Core/Bios.h"
#include "Core/RAM.h"
#include "Core/MemoryControl.h"
#include "Core/SPU.h"
#include "Core/PIO.h"
#include "Core/InterruptControl.h"


using namespace esx;

int main(int argc, char** argv) {
	LoggingSpecifications specs;
	specs.FilePath = ESX_TEXT("");

	LoggingSystem::Start(specs);

	R3000 cpu;
	RAM mainRAM;
	MemoryControl memoryControl;
	InterruptControl interruptControl;
	SPU spu;
	PIO pio;
	Bios bios(ESX_TEXT("scph1001.bin"));

	Bus root(ESX_TEXT("Root"));

	root.connectDevice(&cpu);
	root.connectDevice(&bios);
	root.connectDevice(&mainRAM);
	root.connectDevice(&memoryControl);
	root.connectDevice(&spu);
	root.connectDevice(&pio);
	root.connectDevice(&interruptControl);

	while (true) {
		cpu.clock();
	}

	LoggingSystem::Shutdown();

	return 0;
}