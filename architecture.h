#ifndef architecture
#define architecture

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <limits.h>


#define BOOTSTRAP_LOC 0
#define MEM_RESERVE_SIZE 200
#define MEM_SIZE 8192
#define REG_SIZE 10
#define MAX_INST 1000
#define ZERO_VALUE 0
#define STACK_END 5000

struct mem{
	int data_memory[MEM_SIZE];
	char* instruction_memory[MAX_INST];
};

struct regis{
	int reg[REG_SIZE];
	//stack and base pointer
	int stackptr;
	int baseptr;
	int MAR;
	int MDR;
	int retrnaddr;
	int zeroreg;
	int ProgCounter;
};

struct flags_struct{
	bool zero;
	bool overflow;
	bool carry;
	bool sign;
};

struct instruction_format{
	char opcode;
	char rs[2];
	char rt[2];
	char rd[2];
	char shift[6];
	char displace[5];
	char immediate[10];   // exclusive type I
	char address[6];
	char type;
};


struct entry_j {
	char *key;
	char *value;
	struct entry_j *next;
};



struct jumptable_s {
	int size;
	struct entry_j **table;
};

typedef struct entry_j entry_t;
typedef struct jumptable_s jumptable_t;

jumptable_t* jt_create( int size);
int jt_hash( jumptable_t *jumptable, char *key );
entry_t *jt_newpair( char *key, char *value );
void jt_set( jumptable_t *jumptable, char *key, char *value );
char *jt_get( jumptable_t *jumptable, char *key );


#endif
