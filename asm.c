#include "common.h"

unsigned short offset = 0;
unsigned short pc = 0;
unsigned short memory[256];
unsigned short registers[16];

const char* DELIM = " \n\r\t,";

int is_hex_digit(char c) {
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

int hex_digit_to_i(char c) {
	if(c >= '0' && c <= '9')
		return c - '0';
	if(c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if(c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return 0;
}

enum {
	TYPE_IMMEDIATE,
	TYPE_IMMEDIATE_MEMORY,
	TYPE_REGISTER_MEMORY,
	TYPE_REGISTER
};

typedef struct {
	int type; // 0 = immediate, 1 = immediate memory, 2 = register memory, 3 = register
	unsigned short value;
} argument;

int parse_arguments(argument* list) {
	for(int i = 0; i < 3; ++i) {
		char* v1 = strtok(NULL, DELIM);
		if(!v1) {
			return i;
		}
		int ismem = 0;
		if(*v1 == '[') {
			ismem = 1;
			v1++;
		}
		
		if(*v1 == 'r') {
			if(!is_hex_digit(v1[1])) {
				printf("Expected hex digit after 'r'\n"); return -1;
			}
			list[i].value = hex_digit_to_i(v1[1]);
			list[i].type = ismem ? 2 : 3;
		} else {
			unsigned short value;
			sscanf(v1, "%hx", &value);
			list[i].value = value;
			list[i].type = ismem ? 1 : 0;
		}
	}
	return 3;
}

unsigned short make_reg_inst(int opcode, int d, int s, int t) {
	return 	  ((opcode & 0xF) << 12)
			| ((d & 0xF) << 8)
			| ((s & 0xF) << 4)
			| ((t & 0xF) << 0);
}

unsigned short make_addr_inst(int opcode, int d, int addr) {
	return 	  ((opcode & 0xF) << 12)
			| ((d & 0xF) << 8)
			| ((addr & 0xFff) << 0);
}

void set_reg_inst(int opcode, int d, int s, int t) {
	memory[offset++] = make_reg_inst(opcode, d, s, t);
}

void set_addr_inst(int opcode, int d, int addr) {
	memory[offset++] = make_addr_inst(opcode, d, addr);
}

void set_imm(unsigned short value) {
	memory[offset++] = value;
}

int main(int argc, char* argv[]) {
	if(argc != 2) {
		printf("Usage: ./asm <filename>\n");
		return 1;
	}
	FILE* f = fopen(argv[1], "r");
	if(!f) {
		printf("Could not open '%s'\n", argv[1]);
		return 1;
	}
	
	char* line = NULL; size_t size = 0;
	int linecount = 0;
	while(getline(&line, &size, f) >= 0) {
		linecount++;
		char* tok = strtok(line, DELIM);
		if(!tok || !*tok || *tok == ';') continue;
			
		if(*tok == '.') {
			if(streq(tok, ".offset")) {
				char* v1 = strtok(NULL, DELIM);
				if(!v1) {
					//TODO error handling
				}
				sscanf(v1, "%hx", &offset);
			} else if(streq(tok, ".pc")) {
				char* v1 = strtok(NULL, DELIM);
				if(!v1) {
					//TODO error handling
				}
				sscanf(v1, "%hx", &pc);
			} else if(tok[1] == 'r' && is_hex_digit(tok[2]) && tok[3] == 0) {
				char* v1 = strtok(NULL, DELIM);
				unsigned short value;
				sscanf(v1, "%hx", &value);
				unsigned char r = hex_digit_to_i(tok[2]);
				registers[r] = value;
			} else if(streq(tok, ".immediate")) {
				char* v1;
				while(v1 = strtok(NULL, DELIM)) {
					unsigned short value;
					sscanf(v1, "%hx", &value);
					set_imm(value);
				}
			} else {
				printf("Error: Unknown token '%s'\n", tok);
				return 1;
			}
			continue;
		}
		
		argument args[3];
		int num = parse_arguments(args);
		if(num == -1) {
			printf("Error parsing arguments\n");
			return 1;
		}
#define REQUIRES(c, err) if(!(c)) {printf("ERROR: " err " on line %d\n", linecount); return 1; }
#define ARITH_INST(i, n) else if(streq(tok, n)) { REQUIRES(num == 3 && args[0].type == TYPE_REGISTER && args[1].type == TYPE_REGISTER && args[2].type == TYPE_REGISTER, n " requires three register operands"); set_reg_inst(i, args[0].value, args[1].value, args[2].value); }
		if(streq(tok, "hlt")) {
			set_reg_inst(0, 0, 0, 0);
		} 
			ARITH_INST(1, "add")
			ARITH_INST(2, "sub")
			ARITH_INST(3, "and")
			ARITH_INST(4, "xor")
			ARITH_INST(5, "shl")
			ARITH_INST(6, "shr")
		
		else if(streq(tok, "mov")) {
			REQUIRES(num == 2, "mov requires exactly two arguments");
			REQUIRES(args[0].type != TYPE_IMMEDIATE, "destination to mov cannot be immediate value");
			if(args[0].type == TYPE_IMMEDIATE_MEMORY || args[0].type == TYPE_REGISTER_MEMORY) {
				REQUIRES(args[1].type == TYPE_REGISTER, "mov requires one register argument");
				if(args[0].type == TYPE_IMMEDIATE_MEMORY) {
					set_addr_inst(9, args[1].value, args[0].value);
				} else {
					set_reg_inst(11, args[1].value, 0, args[0].value);
				}
				
			} else {
				REQUIRES(args[1].value != TYPE_REGISTER, "cannot move from register to register");
				if(args[1].type == TYPE_IMMEDIATE) {
					set_addr_inst(7, args[0].value, args[1].value);
				} else if(args[1].type == TYPE_IMMEDIATE_MEMORY) {
					set_addr_inst(8, args[0].value, args[1].value);
				} else {
					set_reg_inst(10, args[0].value, 0, args[1].value);
				}
			}
			
		}
		else if(streq(tok, "load")) {
			REQUIRES(num == 2 && args[0].type == TYPE_REGISTER && args[1].type == TYPE_IMMEDIATE_MEMORY, "load requires one register and one immediate memory argument");
			set_addr_inst(8, args[0].value, args[1].value);
		}
		else if(streq(tok, "store")) {
			REQUIRES(num == 2 && args[0].type == TYPE_IMMEDIATE_MEMORY && args[1].type == TYPE_REGISTER, "store requires one immediate memory and one register argument");
			set_addr_inst(9, args[1].value, args[0].value);
		}
		else if(streq(tok, "loadi")) {
			REQUIRES(num == 2 && args[0].type == TYPE_REGISTER && args[1].type == TYPE_REGISTER_MEMORY, "loadi requires one register and one register memory argument");
			set_addr_inst(10, args[0].value, args[1].value);
		}
		else if(streq(tok, "storei")) {
			REQUIRES(num == 2 && args[0].type == TYPE_REGISTER_MEMORY && args[1].type == TYPE_REGISTER, "storei requires one register memory and one register argument");
			set_addr_inst(11, args[1].value, args[0].value);
		}
		else if(streq(tok, "brz")) {
			REQUIRES(num == 2 && args[0].type == TYPE_REGISTER && args[1].type == TYPE_IMMEDIATE, "brz requires one register and one immediate argument");
			set_addr_inst(12, args[0].value, args[1].value);
		}
		else if(streq(tok, "brp")) {
			REQUIRES(num == 2 && args[0].type == TYPE_REGISTER && args[1].type == TYPE_IMMEDIATE, "brz requires one register and one immediate argument");
			set_addr_inst(13, args[0].value, args[1].value);
		}
		else if(streq(tok, "jmp")) {
			REQUIRES(num == 1 && args[0].type == TYPE_REGISTER, "jmp requires one register argument");
			set_reg_inst(14, args[0].value, 0, 0);
		}
		else if(streq(tok, "call")) {
			REQUIRES(num == 2 && args[0].type == TYPE_REGISTER && args[1].type == TYPE_IMMEDIATE, "call requires one register and one immediate argument");
			set_addr_inst(15, args[0].value, args[1].value);
		}
		else {
			printf("Unknown instruction '%s'\n", tok);
			return 1;
		}
	}
	
	const char* fn = strtok(argv[1], ".");
	if(!fn) fn = "out";
	
	char buffer[128];
	sprintf(buffer, "%s.toy", fn);
	
	FILE* out = fopen(buffer, "wb");
	if(!out) {
		printf("Error: Could not open '%s' for writing\n", buffer);
		return 1;
	}
	
	fwrite(&pc, sizeof(pc), 1, out);
	fwrite(&registers, sizeof(registers), 1, out);
	fwrite(&memory, sizeof(memory), 1, out);
	fclose(out);
	printf("Output written to '%s'\n", buffer);
}