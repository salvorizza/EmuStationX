#include <iostream>

#include "Base/Bus.h"
#include "Core/R3000.h"

using namespace esx;

int main(int argc, char** argv) {
	R3000 cpu;

	Bus root("Root");
	root.connectDevice(&cpu);

	while (true) {
		cpu.clock();
	}

	return 0;
}