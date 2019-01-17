#include "config.h"
#include "cpu.h"


#define MOV 0
#define NOT 1
#define OR 2
#define NOR 3
#define AND 4
#define NAND 5
#define XOR 6
#define XNOR 7
#define NEG 8
#define ADD 9
#define SUB 10

#define LT 11
#define NLT 12
#define SLT 13
#define NSLT 14

#define SL 15
#define SR 16
#define SSL 17
#define SSR 18

#define REP 19
#define MUL 20
#define DIV 21

#define STO8 22
#define STO16 23
#define STO32 24
#define LOD8 25
#define LOD16 26
#define LOD32 27

#define DONE 28

typedef struct {
	uint8_t opcode;

	// condition enable
	uint8_t ce;
	// condition invert
	uint8_t ci;
	uint8_t cond;

	uint8_t dest1;
	uint8_t dest0;
	uint8_t src1;
	uint8_t src0;

	// writeback to dest1
	uint8_t w1;
	// writeback to dest0
	uint8_t w0;

	// immediate for src1
	uint8_t i1;
	// immediate for src0
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
	out.dest0 = (inst >> 12) & 0xf;
	out.src1 = (inst >> 8) & 0xf;
	out.src0 = (inst >> 4) & 0xf;
	out.w1 = (inst >> 3) & 1;
	out.w0 = (inst >> 2) & 1;
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
		// if program counter is outside of
		// memory
		// return so no segfault
		if (regs[15] >= MEMORY_SIZE) {
			return;
		}

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
		uint32_t dest0 = 0;
		uint32_t dest1 = 0;

		// inputs
		uint32_t src0 = regs[inst.src0];
		uint32_t src1 = regs[inst.src1];

		// immediates
		if (inst.i0) {
			src0 = cpu_load32(memory, regs[15]);
			regs[15] += 1;
		}

		if (inst.i1) {
			src1 = cpu_load32(memory, regs[15]);
			regs[15] += 1;
		}

		switch (inst.opcode) {
			case MOV:
				dest0 = src0;
				dest1 = src1;
				break;

			case NOT:
				dest0 = ~src0;
				dest1 = ~src1;
				break;

			case OR:
				dest0 = src0 | src1;
				//dest1 = 0;
				break;

			case NOR:
				dest0 = ~(src0 | src1);
				//dest1 = 0xffffffff;
				break;

			case AND:
				dest0 = src0 & src1;
				//dest1 = 0;
				break;

			case NAND:
				dest0 = ~(src0 & src1);
				//dest1 = 0xffffffff;
				break;

			case XOR:
				dest0 = src0 ^ src1;
				//dest1 = 0;
				break;

			case XNOR:
				dest0 = ~(src0 ^ src1);
				//dest1 = 0xffffffff;
				break;

			case NEG:
				dest0 = -src0;
				//dest1 = 
				break;

			case ADD:
				dest0 = src0 + src1;

				if (src0 > 0xffffffff - src1) {
					dest1 = 1;
				} else {
					dest1 = 0;
				}
				break;

			case SUB:
				dest0 = src0 - src1;

				if (src0 < src1) {
					dest1 = 0xffffffff;
				}
				break;

			case LT:
				if (src0 < src1) {
					dest0 = 0xffffffff;
				}
				break;

			case NLT:
				if (src0 >= src1) {
					dest0 = 0xffffffff;
				}
				break;

			case SLT:
				if ((src0 ^ 0x80000000) < (src1 ^ 0x80000000)) {
					dest0 = 0xffffffff;
				}
				break;

			case NSLT:
				if ((src0 ^ 0x80000000) >= (src1 ^ 0x80000000)) {
					dest0 = 0xffffffff;
				}
				break;

			case SL: {
				uint64_t dest = ((uint64_t) src0) << ((uint8_t) src1);
				dest0 = dest;
				dest1 = dest >> 32;
				break;
			}

			case SR: {
				uint64_t dest = (((uint64_t) src0) << 32) >> ((uint8_t) src1);
				dest0 = dest;
				dest1 = dest >> 32;
				break;
			}

			case SSL: {
				uint64_t dest = (*(int64_t*) &src0) << ((uint8_t) src1);
				dest0 = dest;
				dest1 = dest >> 32;
				break;
			}

			case SSR: {
				uint64_t dest = ((*(int64_t*) &src0) << 32) >> ((uint8_t) src1);
				dest0 = dest;
				dest1 = dest >> 32;
				break;
			}

			case REP:
				if (src0 == 0) {
					dest0 = 0xffffffff;
				} else {
					dest0 = 1 / src0;
				}
				break;

			case MUL: {
				uint64_t dest = ((uint64_t) src0) * ((uint64_t) src1);
				dest0 = dest;
				dest1 = dest >> 32;
				break;
			}

			case DIV:
				dest0 = src0 % src1;
				dest1 = src0 / src1;
				break;

			case STO8:
				dest0 = src0;
				dest1 = src1;
				cpu_store8(memory, src0, src1);
				break;

			case STO16:
				dest0 = src0;
				dest1 = src1;
				cpu_store16(memory, src0, src1);
				break;

			case STO32:
				dest0 = src0;
				dest1 = src1;
				cpu_store32(memory, src0, src1);
				break;

			case LOD8:
				dest0 = cpu_load8(memory, src0);
				dest1 = src1;
				break;

			case LOD16:
				dest0 = cpu_load16(memory, src0);
				dest1 = src1;
				break;

			case LOD32:
				dest0 = cpu_load32(memory, src0);
				dest1 = src1;
				break;

			case DONE:
				return;

			default:
				printf("invalid opcode %i\n", inst.opcode);
				return;
		}

		if (inst.w0) {
			regs[inst.dest0] = dest0;
		}

		if (inst.w1) {
			regs[inst.dest1] = dest1;
		}
	}
}


