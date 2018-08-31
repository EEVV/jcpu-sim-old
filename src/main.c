#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "config.h"
#include "cpu.h"


static size_t memory_bytes = MEMORY_SIZE << 2;


// memory (both data and instruction)
uint8_t memory[MEMORY_SIZE << 2] = {0};

// registers
uint32_t regs[16] = {0};

void print_regs(void) {
	puts("reg:");

	int i = 0;
	while (i < 16) {
		printf("%02x %02x %02x %02x\n", regs[i] >> 24, (regs[i] >> 16) & 0xff, (regs[i] >> 8) & 0xff, regs[i] & 0xff);

		i += 1;
	}
}

void print_memory(void) {
	puts("mem:");

	int i = 0;
	while (i < memory_bytes) {
		if ((i+1) % 4) {
			printf("%02x ", memory[i]);
		} else {
			printf("%02x\n", memory[i]);
		}

		i += 1;
	}
}

int main(int argc, char* argv[]) {
	if (argc == 1) {
		fputs("missing path argument\n", stderr);

		return -1;
	} else if (argc > 2) {
		fputs("too many arguments\n", stderr);

		return -1;
	}

	FILE* file = fopen(argv[1], "r");

	if (!file) {
		fputs("invalid file\n", stderr);

		return -1;
	}

	fseek(file, 0, SEEK_END);
	size_t len = ftell(file);
	rewind(file);

	if (len > memory_bytes) {
		fputs("file too big\n", stderr);

		return -1;
	}

	fread(memory, len, 1, file);
	fclose(file);

	cpu_run(memory, regs);

	print_regs();
	puts("");
	print_memory();

	return 0;
}