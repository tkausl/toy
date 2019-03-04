#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

size_t getdelim(char **linep, size_t *n, int delim, FILE *fp){
    int ch;
    size_t i = 0;
    if(!linep || !n || !fp){
        errno = EINVAL;
        return -1;
    }
    if(*linep == NULL){
        if(NULL==(*linep = malloc(*n=128))){
            *n = 0;
            errno = ENOMEM;
            return -1;
        }
    }
    while((ch = fgetc(fp)) != EOF){
        if(i + 1 >= *n){
            char *temp = realloc(*linep, *n + 128);
            if(!temp){
                errno = ENOMEM;
                return -1;
            }
            *n += 128;
            *linep = temp;
        }
        (*linep)[i++] = ch;
        if(ch == delim)
            break;
    }
    (*linep)[i] = '\0';
    return !i && ch == EOF ? -1 : i;
}
size_t getline(char **linep, size_t *n, FILE *fp){
    return getdelim(linep, n, '\n', fp);
}

int streq(const char* a, const char* b) {
	return strcmp(a, b) == 0;
}

unsigned short memory[256];
unsigned short registers[16];
unsigned short pc;

void printmemory();
void printregisters();
void printhelp();
// Returns 1 when everything went well and the machine is still running,
// 0 when the machine stopped (HALT was executed),
// -1 on error
int runstep();

int main(int argc, char* argv[]) {
	if(argc == 2) {
		FILE* f = fopen(argv[1], "rb");
		if(!f) {
			printf("Could not open file '%s'\n", argv[1]);
			return 1;
		}
		
		fread(&pc, sizeof(pc), 1, f);
		fread(&registers, sizeof(registers), 1, f);
		fread(&memory, sizeof(memory), 1, f);
		fclose(f);
		
		while(runstep() == 1);
		
		printf("\n\nPC = %02X\nRegisters:\n", pc);
		printregisters();
		printf("\nMemory:\n");
		printmemory();
		return 0;
	}
	
	int arg0, arg1;
	int num;
	char* line = NULL; size_t linelen = 0;
	while(getline(&line, &linelen, stdin) >= 0) {
		char* tok = strtok(line, " \n");
		if(!tok || !*tok) continue;
		
		if(streq(tok, "m")) {
			char* v1 = strtok(NULL, " \n");
			if(v1) {
				int addr;
				sscanf(v1, "%x", &addr);
				while(v1 = strtok(NULL, " \n")) {
					int value;
					sscanf(v1, "%x", &value);
					memory[addr] = value;
					addr++;
				}
			} else {
				printmemory();
			}
		} else if(streq(tok, "r")) {
			char* v1 = strtok(NULL, "\n");
			if(v1) {
				int addr, val;
				sscanf(v1, "%x %x", &addr, &val);
				registers[addr] = val;
			} else {
				printregisters();
			}
		} else if(streq(tok, "p")) {
			char* v1 = strtok(NULL, "\n");
			if(v1) {
				int addr;
				sscanf(v1, "%x", &addr);
				pc = addr;
			} else {
				printf("PC = %02X\n", pc);
			}
		} else if(streq(tok, "s")) {
			char* v1 = strtok(NULL, "\n");
			if(v1) {
				int num;
				sscanf(v1, "%d", &num);
				while(num--) {
					int result = runstep();
					if(result != 1) break;
				}
			} else {
				runstep();
			}
		} else if(streq(tok, "h")) {
			printhelp();
		} else if(streq(tok, "store")) {
			char* v1 = strtok(NULL, "\n");
			if(!v1) {
				printf("Usage: strore <filename>\n");
				continue;
			}
			FILE* f = fopen(v1, "wb");
			if(!f) {
				printf("Could not open file '%s'\n", v1);
				continue;
			}
			fwrite(&pc, sizeof(pc), 1, f);
			fwrite(&registers, sizeof(registers), 1, f);
			fwrite(&memory, sizeof(memory), 1, f);
			fclose(f);
			printf("State stored in '%s'\n", v1);
		} else if(streq(tok, "load")) {
			char* v1 = strtok(NULL, "\n");
			if(!v1) {
				printf("Usage: load <filename>\n");
				continue;
			}
			FILE* f = fopen(v1, "rb");
			if(!f) {
				printf("Could not open file '%s'\n", v1);
				continue;
			}
			fread(&pc, sizeof(pc), 1, f);
			fread(&registers, sizeof(registers), 1, f);
			fread(&memory, sizeof(memory), 1, f);
			fclose(f);
			printf("State loaded from '%s'\n", v1);
		}
	}
	printf("SORRY\n");
}

#define M memory
#define R registers

typedef struct {
	unsigned char opcode;
	unsigned char d;
	unsigned char s;
	unsigned char t;
	unsigned short addr;
} instruction;

void printhelp() {
	printf("\nAvailable commands:\n"
		"h\n  prints this help message\n\n"
		"m\n  displays the memory\n\n"
		"m <addr> <value> [<value>...]\n  sets one or more values, beginning at <addr>\n\n"
		"r\n  prints the state of the registers\n\n"
		"r <addr> <value>\n  sets the state of register <addr> to <value>\n\n"
		"p\n  prints the PC\n\n"
		"p <value>\n  sets the PC\n\n"
		"s\n  executes one instruction from M[PC] and increments the PC\n\n"
		"s <num>\n  executes <num> instructions. Stops at HALT\n\n"
		"store <filename>\n  store the current state in <filename>\n\n"
		"load <filename>\n  load the state stored in <filename>\n\n"
	);
}

void decode_instruction(instruction* result, unsigned short i) {
	result->opcode = (i >> 12) & 0xF;
	result->d = (i >> 8) & 0xF;
	result->s = (i >> 4) & 0xF;
	result->t = (i >> 0) & 0xF;
	result->addr = i & 0xFF;
}

#define ARITH(name, symbol) printf(#name " R[%X] <- R[%X] " #symbol " R[%X]\n", inst.d, inst.s, inst.t); registers[inst.d] = registers[inst.s] symbol registers[inst.t]

int runstep() {
	printf("%02X - %04X: ", pc, M[pc]);
	instruction inst;
	decode_instruction(&inst, M[pc]);
	
	pc++;
	switch(inst.opcode) {
		case 0:
			printf("HALT\n");
			pc--;
			return 0;
		break;
		case 1:
			ARITH(ADD, +);
		break;
		case 2:
			ARITH(SUB, -);
		break;
		case 3:
			ARITH(AND, &);
		break;
		case 4:
			ARITH(XOR, ^);
		break;
		case 5:
			ARITH(LSH, <<);
		break;
		case 6:
			ARITH(RSH, >>);
		break;
		case 7:
			printf("R[%X] <- %02X\n", inst.d, inst.addr);
			R[inst.d] = inst.addr;
		break;
		case 8:
			printf("R[%X] <- M[%02X]\n", inst.d, inst.addr);
			R[inst.d] = M[inst.addr];
		break;
		case 9:
			printf("M[%02X] <- R[%X]\n", inst.addr, inst.d);
			M[inst.addr] = R[inst.d];
		break;
		case 10:
			printf("R[%X] <- M[R[%X]]\n", inst.d, inst.t);
			R[inst.d] = M[R[inst.t]];
		break;
		case 11:
			printf("M[R[%X]] <- R[%X]\n", inst.t, inst.d);
			M[R[inst.t]] = R[inst.d];
		break;
		case 12:
			printf("BRZ R[%X] to %02X\n", inst.d, inst.addr);
			if(R[inst.d] == 0) pc = inst.addr;
		break;
		case 13:
			printf("BRP R[%X] to %02X\n", inst.d, inst.addr);
			if(((signed short)R[inst.d]) > 0) pc = inst.addr;
		break;
		case 14:
			printf("JMP R[%X]\n", inst.d);
			pc = R[inst.d];
		break;
		case 15:
			printf("CALL R[%X] to %02X\n", inst.d, inst.addr);
			R[inst.d] = pc; pc = inst.addr;
		break;
	}
	return 1;
}

void printregisters() {
	printf("   0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F\n");
	for(int i = 0; i < 16; ++i) {
		printf("%04X ", registers[i]);
	}
	printf("\n");
}

void printmemory() {
	printf("        0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F\n");
	printf("     -------------------------------------------------------------------------------\n");
	for(int i = 0; i < 16; ++i) {
		printf("%02X | ", i * 16);
		for(int j = 0; j < 16; ++j) {
			printf("%04X ", memory[i*16+j]);
		}
		printf("\n");
	}
}