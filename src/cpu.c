#include "cpu.h"


typedef struct {
	uint8_t opcode;

	// condition enable
	uint8_t ce;
	// condition invert
	uint8_t ci;
	// condition
	uint8_t cond;

	// 2nd destination
	uint8_t dest1;
	// 2nd source
	uint8_t src1;
	// 1st destination
	uint8_t dest0;
	// 1st source
	uint8_t src0;

	// priority bit for deciding on data races
	uint8_t p;

	// immediate goes to src0, src1
	uint8_t ib;
	// immediate goes to src0
	uint8_t i1;
	// immediate goes to src1
	uint8_t i0;
} Inst;

// parse an instruction
Inst cpu_inst(uint32_t inst) {
	Inst out;
		
	out.opcode = inst >> 26;
	out.ce = (inst >> 25) & 1;
	out.ci = (inst >> 24) & 1;
	out.cond = (inst >> 20) & 0xf;
	out.dest1 = (inst >> 16) & 0xf;
	out.src1 = (inst >> 12) & 0xf;
	out.dest0 = (inst >> 8) & 0xf;
	out.src0 = (inst >> 4) & 0xf;
	out.p = (inst >> 3) & 1;
	out.ib = (inst >> 2) & 1;
	out.i1 = (inst >> 1) & 1;
	out.i0 = inst & 1;

	return out;
}

void cpu_store8(uint8_t* memory, uint32_t in_addr, uint32_t data) {
	memory[in_addr] = data;
}

void cpu_store16(uint8_t* memory, uint32_t in_addr, uint32_t data) {
	uint32_t addr = in_addr << 1;

	memory[addr] = data;
	memory[addr+1] = data >> 8;
}

void cpu_store32(uint8_t* memory, uint32_t in_addr, uint32_t data) {
	uint32_t addr = in_addr << 2;

	memory[addr] = data;
	memory[addr+1] = data >> 8;
	memory[addr+2] = data >> 16;
	memory[addr+3] = data >> 24;
}

uint8_t cpu_load8(uint8_t* memory, uint32_t in_addr) {
	return memory[in_addr];
}

uint16_t cpu_load16(uint8_t* memory, uint32_t in_addr) {
	uint32_t addr = in_addr << 1;

	return memory[addr] | (memory[addr+1] << 8);
}

uint32_t cpu_load32(uint8_t* memory, uint32_t in_addr) {
	uint32_t addr = in_addr << 2;

	return memory[addr] | (memory[addr+1] << 8) | (memory[addr+2] << 16) | (memory[addr+3] << 24);
}

void cpu_run(uint8_t* memory, uint32_t regs[16]) {
	while (1) {
		Inst inst = cpu_inst(cpu_load32(memory, regs[15]));

		regs[15] += 1;

		// condition check
		if (inst.ce) {
			if (inst.ci) {
				if (regs[inst.cond] > 0) {
					continue;
				}
			} else {
				if (regs[inst.cond] == 0) {
					continue;
				}
			}
		}

		// outputs
		uint32_t dest0;
		uint32_t dest1 = 0;

		// inputs
		uint32_t src0 = 0;
		uint32_t src1 = 0;

		// immediates
		if (inst.ib) {
			src0 = cpu_load32(memory, regs[15]);
			src1 = src0;
			regs[15] += 1;
		} else if (inst.i0 || inst.i1) {
			if (inst.i0) {
				src0 = cpu_load32(memory, regs[15]);
				regs[15] += 1;
			}

			if (inst.i1) {
				src1 = cpu_load32(memory, regs[15]);
				regs[15] += 1;
			}
		} else {
			src0 = regs[inst.src0];
			src1 = regs[inst.src1];
		}

		switch (inst.opcode) {
			// mov
			case 0b000000:
				dest0 = src0;
				dest1 = src1;
				break;

			// not
			case 0b000001:
				dest0 = ~src0;
				dest1 = ~src1;
				break;

			// or
			case 0b000010:
				dest0 = src0 | src1;
				//dest1 = 0;
				break;

			// nor
			case 0b000011:
				dest0 = ~(src0 | src1);
				//dest1 = 0xffffffff;
				break;

			// and
			case 0b000100:
				dest0 = src0 & src1;
				//dest1 = 0;
				break;

			// nand
			case 0b000101:
				dest0 = ~(src0 & src1);
				//dest1 = 0xffffffff;
				break;

			// xor
			case 0b000110:
				dest0 = src0 ^ src1;
				//dest1 = 0;
				break;

			// xnor
			case 0b000111:
				dest0 = ~(src0 ^ src1);
				//dest1 = 0xffffffff;
				break;

			// neg
			case 0b001000:
				dest0 = -src0;
				//dest1 = 

			// add
			case 0b001001:
				dest0 = src0 + src1;

				if (src0 > 0xffffffff - src1) {
					dest1 = 1;
				} else {
					dest1 = 0;
				}
				break;

			// sub
			case 0b001010:
				dest0 = src0 - src1;

				if (src0 < src1) {
					dest1 = 0xffffffff;
				} else {
					dest1 = 0;
				}
				break;

			// lt
			case 0b001011:
				if (src0 < src1) {
					dest0 = 0xffffffff;
				} else {
					dest0 = 0;
				}
				break;

			// lte
			case 0b001100:
				if (src0 <= src1) {
					dest0 = 0xffffffff;
				} else {
					dest0 = 0;
				}
				break;

			// rep
			case 0b001101:
				if (src0 == 0) {
					dest0 = 0xffffffff;
				} else {
					dest0 = 1 / src0;
				}
				break;

			// mul
			case 0b001110: {
				uint64_t dest = ((uint64_t) src0) * ((uint64_t) src1);
				dest0 = dest;
				dest1 = dest >> 32;
				break;
			}

			// div
			case 0b001111:
				dest0 = src0 % src1;
				dest1 = src0 / src1;
				break;

			// sto8
			case 0b010000:
				dest0 = src0;
				dest1 = src1;
				cpu_store8(memory, src0, src1);
				break;

			// sto16
			case 0b010001:
				dest0 = src0;
				dest1 = src1;
				cpu_store16(memory, src0, src1);
				break;

			// sto32
			case 0b010010:
				dest0 = src0;
				dest1 = src1;
				cpu_store32(memory, src0, src1);
				break;

			// lod8
			case 0b010011:
				dest0 = cpu_load8(memory, src0);
				dest1 = src1;
				break;

			// lod16
			case 0b010100:
				dest0 = cpu_load16(memory, src0);
				dest1 = src1;
				break;

			// lod32
			case 0b010101:
				dest0 = cpu_load32(memory, src0);
				dest1 = src1;
				break;

			// hlt
			case 0b010110:
				return;

			default:
				printf("invalid opcode %i\n", inst.opcode);
				return;
		}

		// if there is a data race
		if (inst.dest0 == inst.dest1) {
			// priority bit for data race
			if (inst.p) {
				regs[inst.dest1] = dest1;
			} else {
				regs[inst.dest0] = dest0;
			}
		} else {
			regs[inst.dest0] = dest0;
			regs[inst.dest1] = dest1;
		}
	}
}


