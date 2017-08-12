#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include <limits.h>

#include "architecture.h"
#include "ALU.c"



struct mem memory;

struct regis registr;

struct flags_struct flags;

struct instruction_format instruction_registr;

jumptable_t * jumptable;
//instruction Syntax
//lod  [Register rd],[Register rt]:[Register rs]:[shift]:[displace]
//lod [Register rd], [immediate]
//sto [Register rt]:[Register rs]:[shift]:[displace],[Register rd]
//sto [immediate], [Register rd]
//add [rd],[rs] ---> rd=rs+rd
//add [register rd], [Register rs], [register rt] --> rd=rs+rt
//add rd:[immediate] = rd = rd+immediate
//add [rs]:[immediate]
//sub [rd],[rs] ---> rd=rs-rd
//sub [register rd], [Register rs], [register rt] --> rd=rs-rt
//mul [rd],[rs] ---> rd=rs*rd
//mul [register rd], [Register rs], [register rt] --> rd=rs*rt
//div,mod
//[j/jl] v:[value]
//[j/jl] [label]
//[j/jl] r:[reg]
//jr [reg]
//cmpq [rs],[rd]
//bne [rs],[rd],[label]
//beq [rs],[rd],[label]
//setg [rd]
//setl [rd]
//sete [rd]
//setne [rd]
//sets [rd]
//setns [rd]
//mov [rd],[immediate]
//lea [rd],[rt]:[rs]:[scale]:[displacement]
//and [rs],[rd],[rt]
//or [rs],[rd],[rt]
//not [rd]
//xor [rs],[rd],[rt]
//srl [rs],[shift]
//sll [rs],[shift]


//after the CPU has been turned on
//this function imitates execution of bootstrap process
void boot_CPU(){
    int i;
    srand(time(NULL));

    fprintf(stdout,"Booting CPU.........\n");

    //update program counter with first instruction
    registr.ProgCounter = 0; // first instruction memroy index
    registr.stackptr = 8191;
    registr.baseptr = 8191;
    registr.zeroreg = ZERO_VALUE;

    //change to storing value in binary format
    for(i=MEM_RESERVE_SIZE;i<MEM_SIZE;i++){
        memory.data_memory[i]=(rand()%1000);
    }
    jumptable = jt_create( 256 );
    //sleep(3);
    /** Initialize flags*****/
    flags.zero=false;
    flags.overflow =false;
    flags.carry =false;
    flags.sign =false;
    //CPU has booted
}

char * trimwhitespace(char *str){
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == '\0')  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator
  *(end+1) = '\0';

  return str;
}

/* Create a new jumptable. */
jumptable_t* jt_create( int size ) {

    jumptable_t *jumptable = NULL;
    int i;

    if( size < 1 ) return NULL;

    /* Allocate the table itself. */
    if( ( jumptable = malloc( sizeof( jumptable_t ) ) ) == NULL ) {
        return NULL;
    }

    /* Allocate pointers to the head nodes. */
    if( ( jumptable->table = malloc( sizeof( entry_t * ) * size ) ) == NULL ) {
        return NULL;
    }
    for( i = 0; i < size; i++ ) {
        jumptable->table[i] = NULL;
    }

    jumptable->size = size;

    return jumptable;
}

/* Hash a string for a particular hash table. */
int jt_hash( jumptable_t *jumptable, char *key ) {

    unsigned long int hashval;
    int i = 0;

    /* Convert our string to an integer */
    while( (hashval < ULONG_MAX) && (i < ((int)strlen(key))) ) {

        hashval = hashval << 4;
        hashval += key[ i ];
        i++;
    }
   // printf("%ld\n", (hashval % jumptable->size));

    return (hashval % jumptable->size);
}

/* Create a key-value pair. */
entry_t *jt_newpair( char *key, char *value ) {
    entry_t *newpair;

    if( ( newpair = malloc( sizeof( entry_t ) ) ) == NULL ) {
        return NULL;
    }

    if( ( newpair->key = strdup( key ) ) == NULL ) {
        return NULL;
    }

    if( ( newpair->value = strdup( value ) ) == NULL ) {
        return NULL;
    }

    newpair->next = NULL;

    return newpair;
}

/* Insert a key-value pair into a hash table. */
void jt_set( jumptable_t *jumptable, char *key, char *value ) {
    int bin = 0;
    entry_t *newpair = NULL;
    entry_t *next = NULL;
    entry_t *last = NULL;

    bin = jt_hash( jumptable, key );

    next = jumptable->table[ bin ];

    while( next != NULL && next->key != NULL && strcmp( key, next->key ) > 0 ) {
        last = next;
        next = next->next;
    }

    /* There's already a pair.  Let's replace that string. */
    if( next != NULL && next->key != NULL && strcmp( key, next->key ) == 0 ) {

        free( next->value );
        next->value = strdup( value );

    /* Nope, could't find it.  Time to grow a pair. */
    } else {
        newpair = jt_newpair( key, value );

        /* We're at the start of the linked list in this bin. */
        if( next == jumptable->table[ bin ] ) {
            newpair->next = next;
            jumptable->table[ bin ] = newpair;

        /* We're at the end of the linked list in this bin. */
        } else if ( next == NULL ) {
            last->next = newpair;

        /* We're in the middle of the list. */
        } else  {
            newpair->next = next;
            last->next = newpair;
        }
    }
}

/* Retrieve a key-value pair from a hash table. */
char *jt_get( jumptable_t *jumptable, char *key ) {
    int bin = 0;
    entry_t *pair;

    bin = jt_hash( jumptable, key );

    /* Step through the bin, looking for our value. */
    pair = jumptable->table[ bin ];
    while( pair != NULL && pair->key != NULL && strcmp( key, pair->key ) > 0 ) {
        pair = pair->next;
    }

    /* Did we actually find anything? */
    if( pair == NULL || pair->key == NULL || strcmp( key, pair->key ) != 0 ) {
        return NULL;

    } else {
        return pair->value;
    }

}
/*displays the content of instruction register*/
void printInst(){
    printf("\n--------------------------------------------------------------------\n");
    printf("|op %c|rd %s|rt %s|rs %s|shift %s|displ %s|imm %s|type %c ",instruction_registr.opcode,instruction_registr.rd,instruction_registr.rt, instruction_registr.rs,
                                     instruction_registr.shift, instruction_registr.displace,instruction_registr.immediate,instruction_registr.type);
    printf("\nPC: %d",registr.ProgCounter);
     printf("\n--------------------------------------------------------------------\n\n");
}

/* displays the content of all registers */
void print_function()
{
    printf("========================================CPU Content=======================================");
    printf("\n**REGISTERS**\n\n");
    printf("R0: %3d| R1: %3d| R2: %3d| R3: %3d| R4: %3d| R5: %3d| R6: %3d| R7: %3d| R8: %3d| R9: %3d| \n",registr.reg[0],registr.reg[1],registr.reg[2],registr.reg[3],registr.reg[4],registr.reg[5],registr.reg[6],registr.reg[7],registr.reg[8],registr.reg[9]);
    printf("MAR = %3d| MDR = %3d| PC = %3d| \n",registr.MAR,registr.MDR,registr.ProgCounter);
    printf("Stack Pointer: %3d| Base Pointer: %3d| ",registr.stackptr,registr.baseptr);
    printf("Return Address: %3d| Zero Register: %3d|\n",registr.retrnaddr,registr.zeroreg);
    printf("-----------------------x------------------------------x---------------------------------\n");
    printf("**FLAGS**\n\n");
    printf("Zero: %3d| Overflow: %3d| Carry: %3d| sign: %3d|\n",flags.zero,flags.overflow,flags.carry,flags.sign);
    printf("-----------------------x------------------------------x---------------------------------\n");
    return;
}

/*print the content of stack used */
void print_stack()
{

    printf("**STACK POINTER**\n\n");
    int i;
    for(i=registr.stackptr; i<=registr.baseptr; i++){
        printf("[%d] = %d\t",i,memory.data_memory[i]);
    }
    printf("\n=======================x============================================x=======================\n\n");

}



void load(){
    int memLoc;
    if(instruction_registr.type == 'I'){
            //get memory location from immediate
            sscanf(instruction_registr.immediate, "%d", &memLoc);

    }
    else{

        memLoc=0;
        char *y = instruction_registr.rd;

        if(strcmp(instruction_registr.rt, "z") == 0){
                         memLoc = 0;
             }
        else if(strcmp(instruction_registr.rt, "s") == 0){ // check for stack pointer
            memLoc = registr.stackptr;
        }
        else if(strcmp(instruction_registr.rt, "b") == 0){ //check for base pointer
            memLoc = registr.baseptr;
        }
        else if(strcmp(instruction_registr.rt, "\0") != 0){
            char *y = instruction_registr.rt;
            int regNo = atoi(y);
            memLoc = memLoc + registr.reg[regNo];

        }
        if(strcmp(instruction_registr.rs, "z") == 0){
                         memLoc = memLoc+0;
             }

        else if(strcmp(instruction_registr.rs, "s") == 0){ //extracting rs register value
            memLoc = memLoc + registr.stackptr;
        }
        else if(strcmp(instruction_registr.rs, "b") == 0){
            memLoc = registr.baseptr;
        }
        else if(strcmp(instruction_registr.rs, "\0") != 0){
            char *y = instruction_registr.rs;
            int regNo = atoi(y);

            if(strcmp(instruction_registr.shift,"\0") !=0){ //check for shift
                int shift;
                sscanf(instruction_registr.shift, "%d", &shift);
                if(shift == 0){
                    memLoc = memLoc + registr.reg[regNo];
                }
                else{
                    memLoc = memLoc + mulALU(registr.reg[regNo], shift ,&flags);
                }

            }
            else{
                memLoc = memLoc + registr.reg[regNo];
            }
        }
        if(strcmp(instruction_registr.displace, "\0") != 0){ //check for displacement
            int disp;
            sscanf(instruction_registr.displace, "%d", &disp);

            memLoc = memLoc + disp;

        }

    }
    //update MAR with memory location address
    registr.MAR = memLoc;
    if(registr.MAR <=100 || registr.MAR >8191 ){
        printf("Memory Address out of bounds!!!!!\n");
        exit(1);
    }

    //Update MDR with memory index value
    registr.MDR = memory.data_memory[memLoc];

    //extract the register index from operand1

    if(strcmp(instruction_registr.rd, "s") == 0){
        printf("Only lea instruction can modify stack pointer!!!!!.\n");
        exit(1);
    }
    else if(strcmp(instruction_registr.rd, "b") == 0){
         printf("Only lea instruction can modify stack base pointer!!!!!.\n");
        exit(1);
    }
    else if(strcmp(instruction_registr.rd, "a") == 0){
        registr.retrnaddr=registr.MDR;;
    }else if(strcmp(instruction_registr.rd, "z") == 0){
        printf("Cannot modify zero register!!!!!\n");
        exit(1);
    }
    else{
        char *y = instruction_registr.rd;
        int regNo = atoi(y);
        //Update register with MDR value
        registr.reg[regNo] = registr.MDR;

    }
}

void store(){
    int memLoc;
    if(instruction_registr.type == 'I'){
            //get memory location from immediate
            sscanf(instruction_registr.immediate, "%d", &memLoc);
    }
    else{
            memLoc=0;
            char *y = instruction_registr.rd;
             if(strcmp(instruction_registr.rt, "z") == 0){
                         memLoc = 0;
             }
             else if(strcmp(instruction_registr.rt, "s") == 0){
                         memLoc = registr.stackptr;
            }
            else if(strcmp(instruction_registr.rt, "\0") != 0){
                char *y = instruction_registr.rt;
                int regNo = atoi(y);
                memLoc = memLoc + registr.reg[regNo];
            }
            if(strcmp(instruction_registr.rs, "z") == 0){
                         memLoc = memLoc+0 ;
             }
             else if(strcmp(instruction_registr.rs, "s") == 0){ //extracting rs register value
                         memLoc = memLoc +  registr.stackptr;
            }
            else if(strcmp(instruction_registr.rs, "\0") != 0){
                char *y = instruction_registr.rs;
                int regNo = atoi(y);
                if(strcmp(instruction_registr.shift,"\0") !=0){
                    int shift;
                    sscanf(instruction_registr.shift, "%d", &shift);
                    if(shift == 0){
                        memLoc = memLoc + registr.reg[regNo];
                    }
                    else{
                        memLoc = memLoc + mulALU(registr.reg[regNo], shift ,&flags);
                    }
                }
                else{
                    memLoc = memLoc + registr.reg[regNo];
                }
            }
            if(strcmp(instruction_registr.displace, "\0") != 0){
                int disp;
                sscanf(instruction_registr.displace, "%d", &disp);
                memLoc = memLoc + disp;
            }
    }
            //update MAR with memory location address
            registr.MAR = memLoc;
            if(registr.MAR <=100 || registr.MAR >8191 ){
                printf("Memory Adress out of bounds!!!!!\n");
                exit(1);
            }

            //extract the register index from operand1
            if(strcmp(instruction_registr.rd, "s") == 0){
                        if(registr.MAR < STACK_END ){
                            printf("Stack is full!!!!!\n");
                            exit(1);
                        }
                         registr.MDR = registr.stackptr;
            }else if(strcmp(instruction_registr.rd, "a") == 0){
                registr.MDR  = registr.retrnaddr;
            }else if(strcmp(instruction_registr.rd, "z") == 0){
                registr.MDR  = registr.zeroreg;
            }else if(strcmp(instruction_registr.rd, "b") == 0){
                registr.MDR  = registr.baseptr;
            }
            else{
                char *y = instruction_registr.rd;
                int regNo = atoi(y);
                registr.MDR = registr.reg[regNo];
            }

            //Update MDR with value in the register
            //Update memory location with MDR value
            memory.data_memory[memLoc] = registr.MDR ;
}

void addInstr(){
    if(strcmp(instruction_registr.rd, "z") == 0){
        printf("Cannot modify zero register!!!!!\n");
        exit(1);

    }

    if(instruction_registr.type == 'I'){

            int num1;
            int destReg;
            char *y = instruction_registr.rd;
            destReg = atoi(y);
            if(strcmp(instruction_registr.rs, "s") == 0){
                         num1 = registr.stackptr;
            }
            else if(strcmp(instruction_registr.rs, "z") == 0){
                         num1 = 0;
             }
            else if(strcmp(instruction_registr.rs, "\0") != 0){
                char *z = instruction_registr.rs;
                int regNo = atoi(z);
                num1 = registr.reg[regNo];
            }

            else {

                num1 = registr.reg[destReg];
            }
            int num2;
            sscanf(instruction_registr.immediate, "%d", &num2);

            if(strcmp(instruction_registr.rd, "s") == 0 ){
                        printf("Only lea instruction can modify stack pointer!!!!!.\n");
                        exit(1);
            }
            else if(strcmp(instruction_registr.rd, "b") == 0){
                 printf("Only lea instruction can modify stack base pointer!!!!!.\n");
                exit(1);

            }else if(strcmp(instruction_registr.rd, "z") == 0){
                printf("Cannot modify zero register!!!!!\n");
                exit(1);
            }
            else{
            registr.reg[destReg] = addALU(num1,num2,&flags);
             }

    }
    else{
            int num1;
            int destReg;
            char *y = instruction_registr.rd;
            destReg = atoi(y);
            char *z = instruction_registr.rs;
            int regNo = atoi(z);
            if(strcmp(instruction_registr.rs, "s") == 0){
                         num1 = registr.stackptr;
            }
            else if(strcmp(instruction_registr.rs, "z") == 0){
                         num1 = 0;
            }
            else{
                        num1 = registr.reg[regNo];
            }
            int num2;
            if(strcmp(instruction_registr.rt, "s") == 0){
                         num2 = registr.stackptr;
            }
            else if(strcmp(instruction_registr.rt, "z") == 0){
                         num2 = 0;
            }
            else if(strcmp(instruction_registr.rt, "\0") != 0){
                char *rt = instruction_registr.rt;
                int regNoRt = atoi(rt);
                num2 = registr.reg[regNoRt];
            }
            else if(strcmp(instruction_registr.rd, "s") == 0){
                         num2 = registr.stackptr;

            }
            else{
                num2 = registr.reg[destReg];

            }

             if(strcmp(instruction_registr.rd, "s") == 0 ){
                        printf("Only lea instruction can modify stack pointer!!!!!.\n");
                        exit(1);
            }
            else if(strcmp(instruction_registr.rd, "b") == 0){
                 printf("Only lea instruction can modify stack base pointer!!!!!.\n");
                exit(1);

            }else if(strcmp(instruction_registr.rd, "z") == 0){
                printf("Cannot modify zero register!!!!!\n");
                exit(1);
            }
            else{
            registr.reg[destReg] = addALU(num1,num2,&flags);
             }
    }
}

void subInstr(){
    //zero register as desitnation register error
    if(strcmp(instruction_registr.rd, "z") == 0){
        printf("Cannot modify zero register!!!!!\n");
        exit(1);
    }

    int num1;
    int destReg;
    char *y = instruction_registr.rd;
    destReg = atoi(y);
    //
    if(strcmp(instruction_registr.rs, "s") == 0){
                     num1 = registr.stackptr ;
    }
     else if(strcmp(instruction_registr.rs, "z") == 0){
                     num1 = 0;
    }
    else{
        char *z = instruction_registr.rs;
        int regNo = atoi(z);
        num1 = registr.reg[regNo];
    }

    int num2;
    if(strcmp(instruction_registr.rt, "s") == 0){
                     num2 = registr.stackptr ;
    }
    else if(strcmp(instruction_registr.rt, "z") == 0){
                     num2 = 0;
    }
    else if(strcmp(instruction_registr.rt, "\0") != 0){
        char *rt = instruction_registr.rt;
        int regNoRt = atoi(rt);
        num2 = registr.reg[regNoRt];
    }
    else {
        num2 = registr.reg[destReg];
    }

    if(strcmp(instruction_registr.rd, "s") == 0 ){
                        printf("Only lea instruction can modify stack pointer!!!!!.\n");
                        exit(1);
            }
            else if(strcmp(instruction_registr.rd, "b") == 0){
                 printf("Only lea instruction can modify stack base pointer!!!!!.\n");
                exit(1);

            }else if(strcmp(instruction_registr.rd, "z") == 0){
                printf("Cannot modify zero register!!!!!\n");
                exit(1);
            }
    else{
     registr.reg[destReg] = subALU(num1,num2,&flags);
     }
}

void mulInstr(){
    if(strcmp(instruction_registr.rd, "z") == 0){
    	printf("Cannot modify zero register!!!!!\n");
    	exit(1);
     }
    int num1;
    int destReg;

    char *y = instruction_registr.rd;
    destReg = atoi(y);
    if(strcmp(instruction_registr.rs, "s") == 0){
    	num1 = registr.stackptr ;
    }
    else if(strcmp(instruction_registr.rs, "z") == 0){
		 num1 = 0;
    }
    else{
			char *z = instruction_registr.rs;
			int regNo = atoi(z);
			num1 = registr.reg[regNo];
    }
    int num2;
    if(strcmp(instruction_registr.rt, "s") == 0){
		 num2 = registr.stackptr ;
    }
    else if(strcmp(instruction_registr.rt, "z") == 0){
		 num2 = 0;
    }
    else if(strcmp(instruction_registr.rt, "\0") != 0){
        char *rt = instruction_registr.rt;
        int regNoRt = atoi(rt);
        num2 = registr.reg[regNoRt];
    }
    else {
        num2 = registr.reg[destReg];
    }

     if(strcmp(instruction_registr.rd, "s") == 0 ){
                        printf("Only lea instruction can modify stack pointer!!!!!.\n");
                        exit(1);
            }
            else if(strcmp(instruction_registr.rd, "b") == 0){
                 printf("Only lea instruction can modify stack base pointer!!!!!.\n");
                exit(1);

            }else if(strcmp(instruction_registr.rd, "z") == 0){
                printf("Cannot modify zero register!!!!!\n");
                exit(1);
            }
    else {
        registr.reg[destReg] = mulALU(num1,num2,&flags);
    }
}

void divInstr(){
    if(strcmp(instruction_registr.rd, "z") == 0){
		printf("Cannot modify zero register!!!!!\n");
		exit(1);
     }
    int num1;
    int destReg;
    char *y = instruction_registr.rd;
    destReg = atoi(y);
    if(strcmp(instruction_registr.rs, "s") == 0){
    	num1 = registr.stackptr ;
    }
    else{
		char *z = instruction_registr.rs;
		int regNo = atoi(z);
		num1 = registr.reg[regNo];
	}
    int num2;
    if(strcmp(instruction_registr.rt, "s") == 0){
		 num2 = registr.stackptr ;
    }
    else if(strcmp(instruction_registr.rt, "\0") != 0){
        char *rt = instruction_registr.rt;
        int regNoRt = atoi(rt);
        num2 = registr.reg[regNoRt];
    }
    else {
        num2 = registr.reg[destReg];
    }

     if(strcmp(instruction_registr.rd, "s") == 0 ){
                        printf("Only lea instruction can modify stack pointer!!!!!.\n");
                        exit(1);
            }
            else if(strcmp(instruction_registr.rd, "b") == 0){
                 printf("Only lea instruction can modify stack base pointer!!!!!.\n");
                exit(1);

            }else if(strcmp(instruction_registr.rd, "z") == 0){
                printf("Cannot modify zero register!!!!!\n");
                exit(1);
            }
    else{
        registr.reg[destReg] = divALU(num1,num2,&flags);
    }
}

void modInstr(){

    int num1;
    int destReg;
    char *y = instruction_registr.rd;
    destReg = atoi(y);
    if(strcmp(instruction_registr.rs, "s") == 0){
		 num1 = registr.stackptr ;
    }
    else{
        char *z = instruction_registr.rs;
        int regNo = atoi(z);
        num1 = registr.reg[regNo];

    }
	int num2;
    if(strcmp(instruction_registr.rt, "s") == 0){
		 num2 = registr.stackptr ;
    }
    else if(strcmp(instruction_registr.rt, "\0") != 0){
        char *rt = instruction_registr.rt;
        int regNoRt = atoi(rt);
        num2 = registr.reg[regNoRt];
    }
    else {
        num2 = registr.reg[destReg];
    }

     if(strcmp(instruction_registr.rd, "s") == 0 ){
                        printf("Only lea instruction can modify stack pointer!!!!!.\n");
                        exit(1);
            }
            else if(strcmp(instruction_registr.rd, "b") == 0){
                 printf("Only lea instruction can modify stack base pointer!!!!!.\n");
                exit(1);

            }else if(strcmp(instruction_registr.rd, "z") == 0){
                printf("Cannot modify zero register!!!!!\n");
                exit(1);
            }
    else{
        registr.reg[destReg] = modALU(num1,num2,&flags);
    }
}

void cmpeqInstr(){
    if(strcmp(instruction_registr.rd, "z") == 0){
		printf("Cannot modify zero register!!!!!\n");
		exit(1);
     }

        int num1;
        int destReg;
        char *y = instruction_registr.rd; //get the value of register1
        destReg = atoi(y);
        if(strcmp(instruction_registr.rs, "s") == 0){ //extracting rs register value
			 num1 = registr.stackptr ;
        }
         else if(strcmp(instruction_registr.rs, "z") == 0){
			 num1 = 0;
        }
        else{
            char *z = instruction_registr.rs;
            int regNo = atoi(z);
            num1 = registr.reg[regNo];
        }
        int num2;
        if(strcmp(instruction_registr.rt, "s") == 0){
			 num2 = registr.stackptr ;
        }
        else if(strcmp(instruction_registr.rt, "z") == 0){
			 num2 = 0;
    }
        else if(strcmp(instruction_registr.rt, "\0") != 0){
            char *rt = instruction_registr.rt;
            int regNoRt = atoi(rt);
            num2 = registr.reg[regNoRt];
        }
        else {
            num2 = registr.reg[destReg];
        }

        // call subALU for checking num1- num2
         subALU(num1,num2,&flags);

}

void testqInstr(){
    //call AND
}

void setEInstr(){ //set if equal
    char *z = instruction_registr.rd;
    int regNo = atoi(z);
    if(flags.zero==true){
        registr.reg[regNo] = 1;
    }
    else{
        registr.reg[regNo] = 0;
    }

}

void setNEInstr(){ //set if not eqaul
    char *z = instruction_registr.rd;
    int regNo = atoi(z);
    if(flags.zero==false){
        registr.reg[regNo] = 1;
    }
    else{
        registr.reg[regNo] = 0;
    }
}

void setSInstr(){ //set if unsigned
    char *z = instruction_registr.rd;
    int regNo = atoi(z);
    if(flags.sign==true){
        registr.reg[regNo] = 1;
    }
    else{
        registr.reg[regNo] = 0;
    }
}

void setNSInstr(){ //set if signed
    char *z = instruction_registr.rd;
    int regNo = atoi(z);
    if(flags.sign==false){
        registr.reg[regNo] = 1;
    }
    else{
        registr.reg[regNo] = 0;
    }
}

void setGInstr(){
    char *z = instruction_registr.rd;
    int regNo = atoi(z);
    if(flags.sign==false){
        registr.reg[regNo] = 1;
    }
    else{
        registr.reg[regNo] = 0;
    }
}

void setLInstr(){
    char *z = instruction_registr.rd;
    int regNo = atoi(z);
    if(flags.sign==1){

        registr.reg[regNo] = 1;
    }
    else{

        registr.reg[regNo] = 0;
    }
}

void jumpInstr(){
    char *jmpAddr = instruction_registr.address;
    registr.ProgCounter = atoi(jmpAddr);
}

void leaInstr(){
    int memLoc;

    if(instruction_registr.type == 'I'){
        //get memory location from immediate
        sscanf(instruction_registr.immediate, "%d", &memLoc);

    }
    else{

        memLoc=0;

        if(strcmp(instruction_registr.rt, "s") == 0){
            memLoc = memLoc + registr.stackptr;
        }
        else if(strcmp(instruction_registr.rt, "z") == 0){
            memLoc = memLoc + 0;
        }
        else if(strcmp(instruction_registr.rt, "b") == 0){
            memLoc = memLoc + registr.baseptr;
        }
        else if(strcmp(instruction_registr.rt, "\0") != 0){
            char *y = instruction_registr.rt;
            int regNo = atoi(y);
            memLoc = memLoc + registr.reg[regNo];

        }
        if(strcmp(instruction_registr.rs, "s") == 0){ //extracting rs register value
            memLoc = memLoc + registr.stackptr;
        }
        else if(strcmp(instruction_registr.rs, "z") == 0){
            memLoc = memLoc + 0;
        }
        else if(strcmp(instruction_registr.rs, "b") == 0){
            memLoc = memLoc + registr.baseptr;
        }
        else if(strcmp(instruction_registr.rs, "\0") != 0){
            char *y = instruction_registr.rs;
            int regNo = atoi(y);
            if(strcmp(instruction_registr.shift,"\0") !=0){

                int shift;
                sscanf(instruction_registr.shift, "%d", &shift);
                if(shift == 0){
                    memLoc = memLoc + registr.reg[regNo];

                }
                else{
                    memLoc = memLoc + mulALU(registr.reg[regNo], shift ,&flags);


                }
            }
            else{
                memLoc = memLoc + registr.reg[regNo];

            }
        }
		if(strcmp(instruction_registr.displace, "\0") != 0){
            int disp;
            sscanf(instruction_registr.displace, "%d", &disp);

            memLoc = memLoc + disp;


        }
    }


     //handle desitnation register values
    if(strcmp(instruction_registr.rd, "s") == 0){
        registr.stackptr =memLoc;
    }
    else if(strcmp(instruction_registr.rd, "b") == 0){
        registr.baseptr =memLoc;
    }
    else if(strcmp(instruction_registr.rd, "z") == 0){
        printf("Cannot modify zero register!!!!!\n");
        exit(1);

    }
    else if(strcmp(instruction_registr.rd, "a") == 0){
        registr.retrnaddr =memLoc;
    }
    else{
        char *y = instruction_registr.rd;
        int regNo = atoi(y);
        registr.reg[regNo] = memLoc;

    }

}

void bneInstr(){
    cmpeqInstr();
    if(flags.zero==true)  {

     }
     else{
        char *jmpAddr = instruction_registr.address;
        registr.ProgCounter = atoi(jmpAddr);
     }

}

void beqInstr(){
    cmpeqInstr();
    if(flags.zero==false)  {

     }
     else{
        char *jmpAddr = instruction_registr.address;
        registr.ProgCounter = atoi(jmpAddr);
     }
}

void andInstr(){


                int num1;
                int destReg;
                char *y = instruction_registr.rd;//get the value of register1
                destReg = atoi(y);
                char *z = instruction_registr.rs;//get the value of register2
                if(strcmp(instruction_registr.rd, "z") == 0){
                    printf("Cannot modify zero register!!!!!\n");
                                    exit(1);

                }
                if(strcmp(instruction_registr.rs, "z") == 0){
                    num1=0;
                }
                else{
                    int regNo = atoi(z);
                    num1 = registr.reg[regNo];
                }
                int num2;
                if(strcmp(instruction_registr.rt, "z") == 0){
                    num2=0;
                }
                else if(strcmp(instruction_registr.rt, "\0") != 0){//get the value of destination register if given
                    char *rt = instruction_registr.rt;
                    int regNoRt = atoi(rt);
                    num2 = registr.reg[regNoRt];
                }
                else {
                    num2 = registr.reg[destReg];
                }
                registr.reg[destReg] = andALU(num1,num2,&flags);
        }


void orInstr(){
                    if(strcmp(instruction_registr.rd, "z") == 0){
        printf("Cannot modify zero register!!!!!\n");
                        exit(1);

    }
                    int num1;
                    int destReg;
                    char *y = instruction_registr.rd;//get the value of register1
                    destReg = atoi(y);
                    char *z = instruction_registr.rs;//get the value of register2
                    int regNo = atoi(z);
                    num1 = registr.reg[regNo];
                    int num2;
                    if(strcmp(instruction_registr.rt, "\0") != 0){
                        char *rt = instruction_registr.rt;//get the value of destination register if given
                        int regNoRt = atoi(rt);
                        num2 = registr.reg[regNoRt];
                    }
                    else {
                        num2 = registr.reg[destReg];
                    }
                    registr.reg[destReg] = orALU(num1,num2,&flags);
            }


void xorInstr(){
                    if(strcmp(instruction_registr.rd, "z") == 0){
        printf("Cannot modify zero register!!!!!\n");
                        exit(1);

    }

                    int num1;
                    int destReg;
                    char *y = instruction_registr.rd; //get the value of register1
                    destReg = atoi(y);
                    char *z = instruction_registr.rs; //get the value of register2
                    int regNo = atoi(z);
                    num1 = registr.reg[regNo];
                    int num2;
                    if(strcmp(instruction_registr.rt, "\0") != 0){
                        char *rt = instruction_registr.rt; //get the value of destination register if given
                        int regNoRt = atoi(rt);
                        num2 = registr.reg[regNoRt];
                    }
                    else {
                        num2 = registr.reg[destReg];
                    }
                    registr.reg[destReg] = xorALU(num1,num2,&flags);
            }

void notInstr(){
                    if(strcmp(instruction_registr.rd, "z") == 0){
        printf("Cannot modify zero register!!!!!\n");
                        exit(1);

    }
                    int num1;
                    int destReg;
                    char *y = instruction_registr.rd;
                    destReg = atoi(y);
                    num1 = registr.reg[destReg];  //get the value of register
                    registr.reg[destReg] = notALU(num1,&flags);
            }


void shiftrInstr(){
    if(strcmp(instruction_registr.rd, "z") == 0){
        printf("Cannot modify zero register!!!!!\n");
                        exit(1);

    }

    int num1;
    int destReg;
    char *y = instruction_registr.rd;
    destReg = atoi(y);

        num1 = registr.reg[destReg]; //get the value of register

    int num2;
    sscanf(instruction_registr.shift, "%d", &num2); //get the shift value

    registr.reg[destReg] = shiftrALU(num1,num2,&flags);

}


void shiftlInstr(){
    if(strcmp(instruction_registr.rd, "z") == 0){
        printf("Cannot modify zero register!!!!!\n");
                        exit(1);

    }
    int num1;
    int destReg;
    char *y = instruction_registr.rd;
    destReg = atoi(y);

        num1 = registr.reg[destReg]; //get the value of register

    int num2;
    sscanf(instruction_registr.shift, "%d", &num2); //get the shift value
    registr.reg[destReg] = shiftlALU(num1,num2,&flags);

}


void moveInstr(){
    int src;

    //handle source register or immediate values
    if(instruction_registr.type == 'I'){
        //get memory location from immediate
        sscanf(instruction_registr.immediate, "%d", &src);
    }
    else{
        if(strcmp(instruction_registr.rs, "s") == 0){ //extracting rs register value
            src = registr.stackptr ;
        }
        else if(strcmp(instruction_registr.rs, "b") == 0){
            src = registr.baseptr ;
        }
        else if(strcmp(instruction_registr.rs, "z") == 0){
            src = 0 ;
        }
        else if(strcmp(instruction_registr.rs, "a") == 0){
            src = registr.retrnaddr ;
        }
        else{
            char *z = instruction_registr.rs;
            int regNo = atoi(z);
            src = registr.reg[regNo];

        }
    }

    //handle desitnation register values
    if(strcmp(instruction_registr.rd, "s") == 0){
        registr.stackptr =src;
    }
    else if(strcmp(instruction_registr.rd, "b") == 0){
        registr.baseptr =src;
    }
    else if(strcmp(instruction_registr.rd, "z") == 0){
        printf("Cannot modify zero register!!!!!\n");
        exit(1);

    }
    else if(strcmp(instruction_registr.rd, "a") == 0){
        registr.retrnaddr =src;
    }
    else{
        char *z = instruction_registr.rd;
        int regNo = atoi(z);
        registr.reg[regNo] = src;

    }

}

void jumpreturnInstr(){ //jump to the address label
    if(strcmp(instruction_registr.rd, "s") == 0){
                         registr.ProgCounter =registr.stackptr;
    }
    else if(strcmp(instruction_registr.rd, "b") == 0){
                         registr.ProgCounter = registr.baseptr;
    }
    else if(strcmp(instruction_registr.rd, "a") == 0){
                         registr.ProgCounter = registr.retrnaddr;
    }
    else{
        printf("else");
        char *z = instruction_registr.rd;
        int regNo = atoi(z);
        registr.ProgCounter = registr.reg[regNo];
        printf("registr.reg[regNo] %d%d",regNo,registr.reg[regNo]);
    }
}

void jumplinkInstr(){
	//jump to the label (for jump table)
    char *jmpAddr = instruction_registr.address;
    registr.retrnaddr = registr.ProgCounter;
    registr.ProgCounter = atoi(jmpAddr);
}

void instr_exec(){
    switch(instruction_registr.opcode){
        case '0':

            load();
            break;
        case '1':

            store();
            break;
        case '2':

            addInstr();
            break;
        case '3':

            subInstr();
            break;
        case '4':

            mulInstr();
            break;
        case '5':

            divInstr();
            break;
        case '6':

            modInstr();
            break;
        case '7':

            cmpeqInstr();
            break;
        case '8':
            testqInstr();
            break;
        case '9':

            setEInstr();
            break;
        case 'A':

            setNEInstr();
            break;
        case 'B':

            setSInstr();
            break;
        case 'C':

            setNSInstr();
            break;
        case 'D':

            setGInstr();
            break;
        case 'E':

            setLInstr();
            break;
        case 'F':

            jumpInstr();
            break;
        case 'G':

            leaInstr();
            break;
        case 'H':

            bneInstr();
            break;
        case 'I':

            beqInstr();
            break;
        case 'J':

            andInstr();
            break;
        case 'K':

            orInstr();
            break;
        case 'L':

            xorInstr();
            break;
        case 'M':

            notInstr();
            break;
        case 'N':

            shiftrInstr();
            break;
        case 'O':

            shiftlInstr();
            break;
        case 'P':

            jumpreturnInstr();
            break;
        case 'Q':

            jumplinkInstr();
            break;
        case 'R':

            moveInstr();
            break;
        default:
            printf("default\n" );
    }
}

char reg_Index(char* value){
    char *y;
    //extract the register index from operand2
    y = (char *)malloc(sizeof(value));
    strcpy(y,value);
    y++;
    if(strcmp(y,"sp")==0){
        return 's';     //stack pointer register index
    }else if(strcmp(y,"ad")==0){
        return 'a';     //return address register index
    }
    else if(strcmp(y,"bp")==0){
        return 'b';     //return address register index
    }
    else if(strcmp(y,"z")==0){
        return 'z';     //zero register index
    }else{
        int regNo = atoi(y);
        if((regNo<0) || (regNo>9)){
            return '$';
        }else{
            return y[0];
        }
    }

    return '\0';
}

int operand_Validation(char opcode, char* temp_str) {
    //handle falgs
    if (opcode=='M' ||opcode=='E' || opcode=='9'  ||(opcode=='A')||(opcode=='B')||(opcode=='C')||(opcode=='D')){
        //do not reset flags
    }
    else{
        //reset flags
        flags.zero=false;
        flags.overflow =false;
        flags.carry =false;
        flags.sign =false;
    }

    //extract operands
   if((opcode=='0')||(opcode=='N') || (opcode=='O')){
        //lod, srl, sll
        if (strstr(temp_str, ",") != NULL) {
            // contains , delimiter
            char *split_res[2];

            int split_count = 0;

            char * pch;
            pch = strtok (temp_str,",");
            while (pch != NULL){
                if(split_count==0){
                    split_res[0] = (char *) malloc(sizeof(pch)+1);
                    strcpy(split_res[0],pch);
                }else if(split_count==1){
                    split_res[1] = (char *) malloc(sizeof(pch)+1);
                    strcpy(split_res[1],pch);
                }
                split_count++;
                pch = strtok (NULL, ",");
            }

            if((split_count==1)||(split_count>2)){
                return 1;
            }

            //get register value from operand 1
            if(strstr(split_res[0],"r") !=NULL){
                char value = reg_Index(split_res[0]);

                if((value == '\0')||(value == '$')){
                    return 1;
                }
                memset(instruction_registr.rd,'\0',sizeof(instruction_registr.rd));
                instruction_registr.rd[0] = value;
            }else{
                return 1;
            }

            //get value from operand2
            if(strstr(split_res[1], ":") != NULL){
                //[Register rt]:[Register rs]:[shift]:[displace]
                instruction_registr.type = 'R';
                char *r[4];
                split_count = 0;

                pch = strtok (split_res[1],":");
                while (pch != NULL){

                    if(split_count==0){
                        r[0] = (char *) malloc(sizeof(pch));
                        if((strcmp(pch,"*"))==0){
                            r[0]="\0";
                        }else{
                            strcpy(r[0],pch); //rs
                        }
                    }else if(split_count==1){
                        r[1] = (char *) malloc(sizeof(pch));
                        if((strcmp(pch,"*"))==0){
                            r[1]="\0";
                        }else{
                            strcpy(r[1],pch);   //rt
                        }
                    }else if(split_count==2){
                        r[2] = (char *) malloc(sizeof(pch));
                        if((strcmp(pch,"*"))==0){
                            r[2]=0;
                        }else{
                            strcpy(r[2],pch);   //shift
                        }
                    }else if(split_count==3){
                        r[3] = (char *) malloc(sizeof(pch));
                        if((strcmp(pch,"*"))==0){
                            r[3]=0;
                        }else{
                            strcpy(r[3],pch);   //displace
                        }
                    }

                    pch = strtok (NULL, ":");
                    split_count++;
                }
                if(split_count!=4){
                    return 1;
                }

                if((r[0]=='\0')&&(r[1]=='\0')&&(r[2]=='\0')&&(r[3]=='\0')){
                    return 1;
                }
                //extract operand2 values
                int i;
                for(i=0; i<4; i++){
                    if(i==0){
                        if((strcmp(r[0],"\0"))==0){
                            memset(instruction_registr.rt,'\0',sizeof(instruction_registr.rt));
                        }else{
                            if(strstr(r[0],"r") !=NULL){
                                char value1 = reg_Index(r[0]);

                                if((value1 == '\0')||(value1 == '$')){
                                    return 1;
                                }
                                memset(instruction_registr.rt,'\0',sizeof(instruction_registr.rt));
                                instruction_registr.rt[0]=value1;

                            }else{
                                return 1;
                            }
                        }
                    }else if(i==1){
                        if(r[1]=='\0'){
                            memset(instruction_registr.rs,'\0',sizeof(instruction_registr.rs));
                        }else {

                            if(strstr(r[1],"r") !=NULL){
                                char value1 = reg_Index(r[1]);
                                if((value1 == '\0')||(value1 == '$')){
                                    return 1;
                                }
                                memset(instruction_registr.rs,'\0',sizeof(instruction_registr.rs));
                                instruction_registr.rs[0]=value1;

                            }else{
                                return 1;
                            }
                        }
                    }else if(i==2){
                        if(r[2]==0){
                            memset(instruction_registr.shift,'\0',sizeof(instruction_registr.shift));
                        }else{
                            if((strlen(r[2])<5)&& (strlen(r[2])>0)){

                                memset(instruction_registr.shift,'\0',sizeof(instruction_registr.shift));
                                strcpy(&instruction_registr.shift[0],r[2]);
                            }else{
                                return 1;
                            }
                        }
                    }else if(i==3){
                        if(r[3]==0){
                            memset(instruction_registr.displace,'\0',sizeof(instruction_registr.displace));
                        }else{
                            if((strlen(r[3])<6)&&(strlen(r[3])>0)){
                                memset(instruction_registr.displace,'\0',sizeof(instruction_registr.displace));
                                strcpy(&instruction_registr.displace[0],r[3]);
                                //instruction_registr.displace[5]='\0';
                            }else{
                                return 1;
                            }
                        }
                    }
                }
            }else{
                instruction_registr.type = 'I';
                 if((opcode=='N')||(opcode=='O')){
                    memset(instruction_registr.shift,'\0',sizeof(instruction_registr.shift));
                    strcpy(&instruction_registr.shift[0],split_res[1]);

                }
                else{
                    if((strlen(split_res[1])<10)&&(strlen(split_res[1])>0)){
                        memset(instruction_registr.immediate,'\0',sizeof(instruction_registr.immediate));
                        strcpy(&instruction_registr.immediate[0],split_res[1]);
                        //instruction_registr.immediate[10]='\0';
                    }else{
                        return 1;
                    }
                }
            }
       }else{
           return 1;
       }
   }
   else if(opcode=='1'){

        //store
       if (strstr(temp_str, ",") != NULL) {
            // contains , delimiter
            char *split_res[2];

            int split_count = 0;

            char * pch;
            pch = strtok (temp_str,",");
            while (pch != NULL){
                if(split_count==0){
                    split_res[1] = (char *) malloc(sizeof(pch)+1);
                    strcpy(split_res[1],pch);
                }else if(split_count==1){
                    split_res[0] = (char *) malloc(sizeof(pch)+1);
                    strcpy(split_res[0],pch);
                }
                split_count++;
                pch = strtok (NULL, ",");
            }

            if((split_count==1)||(split_count>2)){
                return 1;
            }

            //get register value from operand 1
            if(strstr(split_res[0],"r") !=NULL){
                char value = reg_Index(split_res[0]);

                if((value=='\0')||(value=='$')){
                    return 1;
                }
                memset(instruction_registr.rd,'\0',sizeof(instruction_registr.rd));
                instruction_registr.rd[0] = value;
            }else{
                return 1;
            }

            //get value from operand2
            if(strstr(split_res[1], ":") != NULL){
                //[Register rt]:[Register rs]:[shift]:[displace]
                instruction_registr.type = 'R';
                char *r[4];
                split_count = 0;

                pch = strtok (split_res[1],":");
                while (pch != NULL){

                    if(split_count==0){
                        r[0] = (char *) malloc(sizeof(pch));
                        if((strcmp(pch,"*"))==0){
                            r[0]="\0";
                        }else{
                            strcpy(r[0],pch); //rs
                        }
                    }else if(split_count==1){
                        r[1] = (char *) malloc(sizeof(pch));
                        if((strcmp(pch,"*"))==0){
                            r[1]="\0";
                        }else{
                            strcpy(r[1],pch);   //rt
                        }
                    }else if(split_count==2){
                        r[2] = (char *) malloc(sizeof(pch));
                        if((strcmp(pch,"*"))==0){
                            r[2]="\0";
                        }else{
                            strcpy(r[2],pch);   //shift
                        }
                    }else if(split_count==3){
                        r[3] = (char *) malloc(sizeof(pch));
                        if((strcmp(pch,"*"))==0){
                            r[3]="\0";
                        }else{
                            strcpy(r[3],pch);   //displace
                        }
                    }

                    pch = strtok (NULL, ":");
                    split_count++;
                }
                if(split_count!=4){
                    return 1;
                }
                if((r[0]=='\0')&&(r[1]=='\0')&&(r[2]=='\0')&&(r[3]=='\0')){
                    return 1;
                }
                //extract operand2 values
                int i;
                for(i=0; i<4; i++){
                    if(i==0){
                        if((strcmp(r[0],"\0"))==0){
                            memset(instruction_registr.rt,'\0',sizeof(instruction_registr.rt));
                        }else{
                            if(strstr(r[0],"r") !=NULL){
                                char value1 = reg_Index(r[0]);

                                if((value1 == '\0')||(value1 == '$')){
                                    return 1;
                                }
                                memset(instruction_registr.rt,'\0',sizeof(instruction_registr.rt));
                                instruction_registr.rt[0]=value1;

                            }else{
                                return 1;
                            }
                        }
                    }else if(i==1){
                        if((strcmp(r[1],"\0"))==0){
                            memset(instruction_registr.rs,'\0',sizeof(instruction_registr.rs));
                        }else{
                            if(strstr(r[1],"r") !=NULL){
                                char value1 = reg_Index(r[1]);
                                if((value1 == '\0')||(value1 == '$')){
                                    return 1;
                                }
                                memset(instruction_registr.rs,'\0',sizeof(instruction_registr.rs));
                                instruction_registr.rs[0]=value1;

                            }else{
                                return 1;
                            }
                        }
                    }else if(i==2){
                        if((strcmp(r[2],"\0"))==0){
                            memset(instruction_registr.shift,'\0',sizeof(instruction_registr.shift));
                        }else{
                            if((strlen(r[2])<5)&& (strlen(r[2])>0)){
                                memset(instruction_registr.shift,'\0',sizeof(instruction_registr.shift));
                                strcpy(&instruction_registr.shift[0],r[2]);
                            }else{
                                return 1;
                            }
                        }
                    }else if(i==3){
                        if((strcmp(r[3],"\0"))==0){
                            memset(instruction_registr.displace,'\0',sizeof(instruction_registr.displace));
                        }else{
                            if((strlen(r[3])<6)&&(strlen(r[3])>0)){
                                memset(instruction_registr.displace,'\0',sizeof(instruction_registr.displace));
                                strcpy(&instruction_registr.displace[0],r[3]);
                                //instruction_registr.displace[5]='\0';
                            }else{
                                return 1;
                            }
                        }
                    }
                }
            }else{
                instruction_registr.type = 'I';
                if((strlen(split_res[1])<10)&&(strlen(split_res[1])>0)){
                    memset(instruction_registr.immediate,'\0',sizeof(instruction_registr.immediate));
                    strcpy(&instruction_registr.immediate[0],split_res[1]);
                    //instruction_registr.immediate[10]='\0';
                }else{
                    return 1;
                }
            }
       }else{
           return 1;
       }
   }
   else if((opcode=='2')||(opcode=='3')||(opcode=='4')||(opcode=='5')||(opcode=='6') ||(opcode=='J')||(opcode=='K')||(opcode=='L') ){
       //add, sub, mul, div, mod, setl, logical and, or, xor
       if (strstr(temp_str, ",") != NULL) {
            // contains , delimiter
            char *split_res[3];

            int split_count = 0;

            char * pch;
            pch = strtok (temp_str,",");
            while (pch != NULL){
                if(split_count==0){
                    split_res[0] = (char *) malloc(sizeof(pch)+1);
                    strcpy(split_res[0],pch);
                }else if(split_count==1){
                    split_res[1] = (char *) malloc(sizeof(pch)+1);
                    strcpy(split_res[1],pch);
                }else if(split_count==2){
                    split_res[2] = (char *) malloc(sizeof(pch)+1);
                    strcpy(split_res[2],pch);
                }
                split_count++;
                pch = strtok (NULL, ",");
            }

            if(split_count==2){
                //rd =  rs,rt
                //get register value from operand 1
                if(strstr(split_res[0],"r") !=NULL){
                    char value = reg_Index(split_res[0]);

                    if((value == '\0')||(value == '$')){
                        return 1;
                    }
                    memset(instruction_registr.rs,'\0',sizeof(instruction_registr.rs));
                    instruction_registr.rs[0] = value;
                }else{
                    return 1;
                }
                if(strstr(split_res[1],"r") !=NULL){
                    char value = reg_Index(split_res[1]);

                    if((value == '\0')||(value == '$')){
                        return 1;
                    }
                    memset(instruction_registr.rt,'\0',sizeof(instruction_registr.rt));
                    instruction_registr.rt[0] = value;
                }else{
                    return 1;
                }

                memset(instruction_registr.rd,'\0',sizeof(instruction_registr.rd));
                instruction_registr.rd[0] = instruction_registr.rs[0];
            }else if(split_count==3){
                //rd,rs,rt
                if(strstr(split_res[0],"r") !=NULL){
                    char value = reg_Index(split_res[0]);

                    if((value == '\0')||(value == '$')){
                        return 1;
                    }
                    memset(instruction_registr.rd,'\0',sizeof(instruction_registr.rd));
                    instruction_registr.rd[0] = value;
                }else{
                    return 1;
                }
                if(strstr(split_res[1],"r") !=NULL){
                    char value = reg_Index(split_res[1]);

                    if((value == '\0')||(value == '$')){
                        return 1;
                    }
                    memset(instruction_registr.rs,'\0',sizeof(instruction_registr.rs));
                    instruction_registr.rs[0] = value;
                }else{
                    return 1;
                }
                if(strstr(split_res[2],"r") !=NULL){
                    char value = reg_Index(split_res[2]);

                    if((value == '\0')||(value == '$')){
                        return 1;
                    }
                    memset(instruction_registr.rt,'\0',sizeof(instruction_registr.rt));
                    instruction_registr.rt[0] = value;
                }else{
                    return 1;
                }
            }else{
                return 1;
            }

            instruction_registr.type = 'R';

       }else if(strstr(temp_str, ":") != NULL){
            //immediate

            char *r[2];
            int split_count = 0;
            char *pch;

            pch = strtok (temp_str,":");
            split_count++;
            while (pch != NULL){

                if(split_count==1){
                    r[0] = (char *) malloc(sizeof(pch));
                    if((strcmp(pch,"*"))==0){
                        r[0]="\0";
                    }else{
                        strcpy(r[0],pch); //rs
                    }
                }else if(split_count==2){
                    r[1] = (char *) malloc(sizeof(pch));
                    if((strcmp(pch,"*"))==0){
                        r[1]="\0";
                    }else{
                        strcpy(r[1],pch);   //imm
                    }
                }

                pch = strtok (NULL, ":");
                split_count++;
            }
            if(split_count!=3){
                return 1;
            }

            if(strcmp(r[0],"\0")!=0){

                if(strstr(r[0],"r") !=NULL){
                    char value1 = reg_Index(r[0]);
                    if((value1 == '\0')||(value1 == '$')){
                        return 1;
                    }
                    memset(instruction_registr.rs,'\0',sizeof(instruction_registr.rs));
                    instruction_registr.rs[0]=value1;

                }else{
                    return 1;
                }
            }else{
                return 1;
            }

            if((strcmp(r[1],"\0"))!=0){

                if((strlen(r[1])<10)&&(strlen(r[1])>0)){
                    memset(instruction_registr.immediate,'\0',sizeof(instruction_registr.immediate));
                    strcpy(&instruction_registr.immediate[0],r[1]);
                    //instruction_registr.immediate[10]='\0';
                }else{
                    return 1;
                }
            }else{
                return 1;
            }

            memset(instruction_registr.rd,'\0',sizeof(instruction_registr.rd));
                    instruction_registr.rd[0]=instruction_registr.rs[0];
            instruction_registr.type = 'I';
       }else{
           return 1;
       }
   }
   else if((opcode=='7')||(opcode=='8')){
        if (strstr(temp_str, ",") != NULL) {
            // contains , delimiter
            char *split_res[2];

            int split_count = 0;

            char * pch;
            pch = strtok (temp_str,",");
            while (pch != NULL){
                if(split_count==0){
                    split_res[0] = (char *) malloc(sizeof(pch)+1);
                    strcpy(split_res[0],pch);
                }else if(split_count==1){
                    split_res[1] = (char *) malloc(sizeof(pch)+1);
                    strcpy(split_res[1],pch);
                }
                split_count++;
                pch = strtok (NULL, ",");
            }

            if(split_count==2){
                //rd =  rs,rt
                //get register value from operand 1
                if(strstr(split_res[0],"r") !=NULL){
                    char value = reg_Index(split_res[0]);

                    if((value == '\0')||(value == '$')){
                        return 1;
                    }
                    memset(instruction_registr.rs,'\0',sizeof(instruction_registr.rs));
                    instruction_registr.rs[0] = value;
                }else{
                    return 1;
                }
                if(strstr(split_res[1],"r") !=NULL){
                    char value = reg_Index(split_res[1]);

                    if((value == '\0')||(value == '$')){
                        return 1;
                    }
                    memset(instruction_registr.rt,'\0',sizeof(instruction_registr.rt));
                    instruction_registr.rt[0] = value;
                }else{
                    return 1;
                }

                memset(instruction_registr.rd,'\0',sizeof(instruction_registr.rd));
                instruction_registr.rd[0] = instruction_registr.rs[0];
            }else{
                return 1;
            }
        }else{
            return 1;
        }
        instruction_registr.type = 'R';

   }

   else if (opcode=='M' ||opcode=='E' || opcode=='9'||(opcode=='A')||(opcode=='B')||(opcode=='C')||(opcode=='D')){
       //logical not
       if(strstr(temp_str,"r") !=NULL){
           char value = reg_Index(temp_str);

           if((value == '\0')||(value == '$')){
               return 1;
           }
           memset(instruction_registr.rd,'\0',sizeof(instruction_registr.rd));
           instruction_registr.rd[0] = value;
           instruction_registr.type = 'R';
       }else{
           return 1;
       }
   }
   else if(opcode == 'G'){

        if (strstr(temp_str, ",") != NULL) {
            // contains , delimiter
            char *split_res[2];

            int split_count = 0;

            char * pch;
            pch = strtok (temp_str,",");
            while (pch != NULL){
                if(split_count==0){
                    split_res[0] = (char *) malloc(sizeof(pch)+1);
                    strcpy(split_res[0],pch);
                }else if(split_count==1){
                    split_res[1] = (char *) malloc(sizeof(pch)+1);
                    strcpy(split_res[1],pch);
                }
                split_count++;
                pch = strtok (NULL, ",");
            }
            if(split_count != 2){
                return 1;
            }

            //get register value from operand 1
            if(strstr(split_res[0],"r") !=NULL){
                char value = reg_Index(split_res[0]);

                if((value == '\0')||(value == '$')){
                    return 1;
                }
                memset(instruction_registr.rd,'\0',sizeof(instruction_registr.rd));
                instruction_registr.rd[0] = value;

            }else{
                return 1;
            }

            //get value from operand2
            if(strstr(split_res[1], ":") != NULL){
                //[Register rt]:[Register rs]:[shift]:[displace]
                instruction_registr.type = 'R';
                char *r[4];
                split_count = 0;

                pch = strtok (split_res[1],":");

                while (pch != NULL){

                    if(split_count==0){
                        r[0] = (char *) malloc(sizeof(pch));
                        if((strcmp(pch,"*"))==0){
                            r[0]="\0";
                        }else{
                            strcpy(r[0],pch); //rt
                        }
                    }else if(split_count==1){
                        r[1] = (char *) malloc(sizeof(pch));
                        if((strcmp(pch,"*"))==0){
                            r[1]="\0";
                        }else{
                            strcpy(r[1],pch);   //rs
                        }
                    }else if(split_count==2){
                        r[2] = (char *) malloc(sizeof(pch));

                        if((strcmp(pch,"*"))==0){
                            r[2]="0";

                                if(r[2]=='\0' ){
                                    printf("0");
                                }
                        }else{
                            strcpy(r[2],pch);   //shift
                        }
                    }else if(split_count==3){
                        r[3] = (char *) malloc(sizeof(pch));
                        if((strcmp(pch,"*"))==0){
                            r[3]="0";
                        }else{
                            strcpy(r[3],pch);   //displace
                        }
                    }

                    pch = strtok (NULL, ":");
                    split_count++;
                }
                if(split_count!=4){
                    return 1;
                }
                if((r[0]=='\0')&&(r[1]=='\0')&&(r[2]=='\0')&&(r[3]=='\0')){
                    return 1;
                }
                //extract operand2 values
                int i;
                for(i=0; i<4; i++){
                    if(i==0){
                        if((strcmp(r[0],"\0"))==0){
                            memset(instruction_registr.rt,'\0',sizeof(instruction_registr.rt));
                        }else{
                            if(strstr(r[0],"r") !=NULL){
                                char value1 = reg_Index(r[0]);

                                if((value1 == '\0')||(value1 == '$')){
                                    return 1;
                                }
                                memset(instruction_registr.rt,'\0',sizeof(instruction_registr.rt));
                                instruction_registr.rt[0]=value1;


                            }else{
                                return 1;
                            }
                        }
                    }else if(i==1){
                        if(r[1]=='\0'){
                            memset(instruction_registr.rs,'\0',sizeof(instruction_registr.rs));
                        }else {

                            if(strstr(r[1],"r") !=NULL){
                                char value1 = reg_Index(r[1]);
                                if((value1 == '\0')||(value1 == '$')){
                                    return 1;
                                }
                                memset(instruction_registr.rs,'\0',sizeof(instruction_registr.rs));
                                instruction_registr.rs[0]=value1;


                            }else{
                                return 1;
                            }
                        }
                    }else if(i==2){
                        printf(" i=2 \n");
                        printf(" r[2] %s \n",r[2]);
                        if(r[2]==0 ){
                            memset(instruction_registr.shift,'\0',sizeof(instruction_registr.shift));
                        }else{


                            if((strlen(r[2])<5)&& (strlen(r[2])>0)){
                                memset(instruction_registr.shift,'\0',sizeof(instruction_registr.shift));
                                strcpy(&instruction_registr.shift[0],r[2]);

                            }else{
                                return 1;
                            }
                        }
                    }else if(i==3){

                       // printf(" r[3] %s \n",r[3]);
                        if(r[3]==0){
                            memset(instruction_registr.displace,'\0',sizeof(instruction_registr.displace));
                        }else{

                            if((strlen(r[3])<6)&&(strlen(r[3])>0)){
                                memset(instruction_registr.displace,'\0',sizeof(instruction_registr.displace));
                                strcpy(&instruction_registr.displace[0],r[3]);

                                //instruction_registr.displace[5]='\0';
                            }else{
                                return 1;
                            }
                        }
                    }
                }

            }else if(strstr(split_res[1], "r") != NULL){
                instruction_registr.type = 'R';
                char value = reg_Index(split_res[1]);

                if((value == '\0')||(value == '$')){
                    return 1;
                }
                memset(instruction_registr.rt,'\0',sizeof(instruction_registr.rt));
                instruction_registr.rt[0] = value;
                memset(instruction_registr.rs,'\0',sizeof(instruction_registr.rs));
                memset(instruction_registr.shift,'\0',sizeof(instruction_registr.shift));
            }else{
                instruction_registr.type = 'I';
                if((strlen(split_res[1])<10)&&(strlen(split_res[1])>0)){
                    memset(instruction_registr.immediate,'\0',sizeof(instruction_registr.immediate));
                    strcpy(&instruction_registr.immediate[0],split_res[1]);
                    //instruction_registr.immediate[10]='\0';
                }else{
                    return 1;
                }
            }
        }
        else{
            return 1;
        }
   }
   else if((opcode=='F')||(opcode == 'Q')){
        //j
        if (strstr(temp_str, ",") != NULL){
            return 1;
        }else if (strstr(temp_str, "v:") != NULL){
            //jump value provided
            temp_str = temp_str+2;
            if(strlen(temp_str)>5){
                return 1;
            }
            strcpy(instruction_registr.address,temp_str);
            instruction_registr.type = 'I';
            return 1;
        }else if(strstr(temp_str, "r:") != NULL){
            //jump register provided
            temp_str = temp_str+2;
            char value = reg_Index(temp_str);

            if((value == '\0')||(value == '$')){
                return 1;
            }

            memset(instruction_registr.rd,'\0',sizeof(instruction_registr.rd));
            instruction_registr.rd[0] = value;
            instruction_registr.type = 'R';

        }else{
            //consider jump label
            char* str = (char*) malloc(sizeof(char)*100);
            str = jt_get(jumptable, temp_str);
            if(str == NULL){
                return 1; //no label found
            }
            strcpy(instruction_registr.address,str);
            instruction_registr.type = 'I';
        }
   }

   else if((opcode == 'H')||(opcode == 'I')){

        //bne bqe $r1,$r2,label
        if (strstr(temp_str, ",") != NULL) {
            // contains , delimiter

            char *split_res[3];

            int split_count = 0;

            char * pch;
            pch = strtok (temp_str,",");
            while (pch != NULL){
                if(split_count==0){
                    split_res[0] = (char *) malloc(sizeof(pch)+1);
                    strcpy(split_res[0],pch);
                }else if(split_count==1){
                    split_res[1] = (char *) malloc(sizeof(pch)+1);
                    strcpy(split_res[1],pch);
                }else if(split_count==2){
                    split_res[2] = (char *) malloc(sizeof(pch)+1);
                    strcpy(split_res[2],pch);
                }
                split_count++;
                pch = strtok (NULL, ",");
            }
            if(split_count==3){
                //rd,rs,rt
                if(strstr(split_res[0],"r") !=NULL){
                    char value = reg_Index(split_res[0]);

                    if((value == '\0')||(value == '$')){
                        return 1;
                    }
                    memset(instruction_registr.rs,'\0',sizeof(instruction_registr.rd));
                    instruction_registr.rs[0] = value;
                }else{
                    return 1;
                }
                if(strstr(split_res[1],"r") !=NULL){
                    char value = reg_Index(split_res[1]);

                    if((value == '\0')||(value == '$')){
                        return 1;
                    }
                    memset(instruction_registr.rt,'\0',sizeof(instruction_registr.rs));
                    instruction_registr.rt[0] = value;
                }else{
                    return 1;
                }
                //label
                if (strstr(split_res[2], "v:") != NULL){
                    //jump value provided
                    char* temp_str = (char*) malloc(strlen(split_res[2])+1);
                    strcpy(temp_str,split_res[2]);
                    temp_str = temp_str+2;
                    if(strlen(temp_str)>5){
                        return 1;
                    }
                    strcpy(instruction_registr.address,temp_str);
                    return 1;
                }else{
                    //consider jump label
                    char* str = (char*) malloc(sizeof(char)*100);
                    str = jt_get(jumptable, split_res[2]);

                    if(str == NULL){
                        return 1; //no label found
                    }

                    strcpy(instruction_registr.address,str);
                }

            }else{
                return 1;
            }

            instruction_registr.type = 'R';
        }else{
            return 1;
        }

   }
   else if(opcode == 'P'){
        //jr
        if(strstr(temp_str,",") !=NULL){
            return 1;
        }
        else if(strstr(temp_str,"r") !=NULL){
            char value = reg_Index(temp_str);

            if((value == '\0')||(value == '$')){
                return 1;
            }

            memset(instruction_registr.rd,'\0',sizeof(instruction_registr.rd));
            instruction_registr.rd[0] = value;
            instruction_registr.type = 'R';
        }
        else{
            return 1;
        }
   }
   else if(opcode == 'R'){
        //mov
        if (strstr(temp_str, ",") != NULL) {
            // contains , delimiter
            char *split_res[2];

            int split_count = 0;

            char * pch;
            pch = strtok (temp_str,",");
            while (pch != NULL){
                if(split_count==0){
                    split_res[0] = (char *) malloc(sizeof(pch)+1);
                    strcpy(split_res[0],pch);
                }else if(split_count==1){
                    split_res[1] = (char *) malloc(sizeof(pch)+1);
                    strcpy(split_res[1],pch);
                }
                split_count++;
                pch = strtok (NULL, ",");
            }

            if(split_count==2){
                //get register value from operand 1
                if(strstr(split_res[0],"r") !=NULL){
                    char value = reg_Index(split_res[0]);

                    if((value == '\0')||(value == '$')){
                        return 1;
                    }
                    memset(instruction_registr.rd,'\0',sizeof(instruction_registr.rd));
                    instruction_registr.rd[0] = value;
                }else{
                    return 1;
                }
                if(strstr(split_res[1],"r") !=NULL){
                    instruction_registr.type = 'R';
                    char value = reg_Index(split_res[1]);

                    if((value == '\0')||(value == '$')){
                        return 1;
                    }
                    memset(instruction_registr.rs,'\0',sizeof(instruction_registr.rs));
                    instruction_registr.rs[0] = value;
                }else{
                    if((strlen(split_res[1])<10)&&(strlen(split_res[1])>0)){
                        instruction_registr.type = 'I';
                        memset(instruction_registr.immediate,'\0',sizeof(instruction_registr.immediate));
                        strcpy(&instruction_registr.immediate[0],split_res[1]);
                    }else{
                        return 1;
                    }
                }
            }else{
                return 1;
            }
       } else{
        return 1;
       }
   }
   else{
        return 1;
   }
   return 0;

}
//validate the instruction and and return opcode
char opcode_Validation(char* str){
    if(strcmp(str,"lod")==0){
        return '0';
    }
    else if(strcmp(str,"sto")==0){
        return '1';
    }
    else if(strcmp(str,"add")==0){
        return '2';
    }
    else if(strcmp(str,"sub")==0){
        return '3';
    }
    else if(strcmp(str,"mul")==0){
        return '4';
    }
    else if(strcmp(str,"div")==0){
        return '5';
    }
    else if(strcmp(str,"mod")==0){
        return '6';
    }
    else if(strcmp(str,"cmpq")==0){
        return '7';
    }
    else if(strcmp(str,"testq")==0){
        return '8';
    }
    else if(strcmp(str,"sete")==0){
        return '9';
    }
    else if(strcmp(str,"setne")==0){
        return 'A';
    }
    else if(strcmp(str,"sets")==0){
        return 'B';
    }
    else if(strcmp(str,"setns")==0){
        return 'C';
    }
    else if(strcmp(str,"setg")==0){
        return 'D';
    }
    else if(strcmp(str,"setl")==0){
        return 'E';
    }
    else if(strcmp(str,"j")==0){
        return 'F';
    }
    else if(strcmp(str,"lea")==0){
        return 'G'; //load address of value
    }
    else if(strcmp(str,"bne")==0){
        return 'H'; //load address of value
    }
    else if(strcmp(str,"beq")==0){
        return 'I'; //load address of value
    }
    else if(strcmp(str,"and")==0){
        return 'J';
    }
    else if(strcmp(str,"or")==0){
        return 'K';
    }
    else if(strcmp(str,"xor")==0){
        return 'L';
    }
    else if(strcmp(str,"not")==0){
        return 'M';
    }
    else if(strcmp(str,"srl")==0){
        return 'N';
    }
    else if(strcmp(str,"sll")==0){
        return 'O';
    }
    else if(strcmp(str,"jr")==0){
        return 'P';
    }
    else if(strcmp(str,"jl")==0){
        return 'Q';
    }
    else if(strcmp(str,"mov")==0){
        return 'R';
    }
    return ' ';
}


int check_Instr(char* temp_str){

    char *split_res[2];

    int split_count = 0;


    char * pch;
    pch = strtok (temp_str," ");
    while (pch != NULL)
    {
        if(split_count==0){
            split_res[0] = (char *) malloc(sizeof(pch));
            strcpy(split_res[0],pch);
        }else if(split_count==1){
            split_res[1] = (char *) malloc(sizeof(pch));
            strcpy(split_res[1],pch);
        }
        split_count++;
        pch = strtok (NULL, " ");
    }

    if((split_count==1)||(split_count>2)){
        return 1;
    }

    //opcode check
    instruction_registr.opcode = opcode_Validation(split_res[0]);
    if(instruction_registr.opcode==' '){
        return 1;
    }

     //operand check
    int operand_rtn;
    operand_rtn = operand_Validation(instruction_registr.opcode, split_res[1]);
    if(operand_rtn==1){
        return 1;
    }

    return 0;
}

void fetch_Instr(){

    char *temp_str = (char *)malloc(100*sizeof(char));
    char *instruction = (char *)malloc(100*sizeof(char));

    for(;registr.ProgCounter<MAX_INST;){

        memset(&instruction_registr,'\0',sizeof(instruction_registr));
        strcpy(temp_str, memory.instruction_memory[registr.ProgCounter]);
        strcpy(instruction,temp_str);

        if(strcmp(temp_str,"end")==0){
            printf("Executed end... goodbye!\n");
            break;
        }

        int check_rtn = check_Instr(temp_str);

        registr.ProgCounter++;

         //let PC point to next instruction;




       if(check_rtn == 1){
            printf("\nError Executing- %s\n",instruction);
            printf("==================================================\n");
            printf("Syntax:\n    lod  [Register rd],[Register rt]:[Register rs]:[shift]:[displace]\n    lod [Register rd], [immediate]\n    sto [Register rt]:[Register rs]:[shift]:[displace],[Register rd]\n    sto [immediate], [Register rd]\n    ");
            printf("add [rd],[rs] ---> rd=rs+rd\n    add [register rd], [Register rs], [register rt] --> rd=rs+rt\n    add rd:[immediate] = rd = rd+immediate\n    add [rs]:[immediate]\n    sub [rd],[rs] ---> rd=rs-rd \n");
			printf("    sub [register rd], [Register rs], [register rt] --> rd=rs-rt\n    mul [rd],[rs] ---> rd=rs*rd\n    mul [register rd], [Register rs], [register rt] --> rd=rs*rt\n    div,mod\n");
			printf("    [j/jl] v:[value]\n    [j/jl] [label]\n    [j/jl] r:[reg]\n    jr [reg] \n");
			printf("    cmpq [rs],[rd]\n    bne [rs],[rd],[label]\n    beq [rs],[rd],[label]\n");
			printf("    setg [rd]\n    setl [rd]\n    sete [rd]\n    setne [rd]\n    sets [rd]\n    setns [rd] \n");
			printf("    mov [rd],[immediate]\n    lea [rd],[rt]:[rs]:[scale]:[displacement]\n");
			printf("    and [rs],[rd],[rt]\n    or [rs],[rd],[rt]\n    not [rd]\n    xor [rs],[rd],[rt]\n    srl [rs],[shift]\n    sll [rs],[shift] \n");
            printf("Use '*' if you are not giving value to any parameter\n");
            printf("Usable Memory Location index: 100-1023\nRegister index Usable [R,r]0-7\n");
            printf("No other special character other than \",\" is used\n");
            printf("===================================================\n\n");
            exit(1);
        }else if(check_rtn == 0){
            printf("Executing- %s",instruction);
            printInst();
            instr_exec();
            print_function();
            print_stack();
        }
    }
}


int main(){

    //memory allocation
    memory.data_memory[BOOTSTRAP_LOC] = 99;
    int j;
    for(j=0; j<MAX_INST; j++){
        memory.instruction_memory[j]=(char *)malloc(100*sizeof(char));
    }

    //*********CPU Execution starts from here**************
    boot_CPU();    //boot the CPU

    fprintf(stdout,".....CPU has booted up!\n");

    printf("\nInitial memory content :\n");
    printf("-------------------------------------------------\n");
    int i;
    for(i=500;i<506;i++){
        printf("memory %d -----> %d|", i, memory.data_memory[i]);
    }
    printf("\n-------------------------------------------------\n");
    //get users to input instructions
    //maximum of 100 instructions can be provided as input
    char* line = (char *)malloc(100*sizeof(char));



    printf("\n_______Welcome_______\n\n");

    while(1) {
        printf("Virtual-Processor:~# ");
        fgets(line, 100, stdin);
        line = trimwhitespace(line);


        FILE *ifp;
        size_t len = 0;
        ssize_t read;
        char* user_input = NULL;

        ifp = fopen(line, "r");

        if (ifp == NULL) {
          fprintf(stderr, "\nError: Can't open input file %s\n",user_input);
        }
        else{
            int count=0;
            while (((read = getline(&user_input, &len, ifp)) != -1)&& (count<=MAX_INST) ){

                if(*user_input != '\n'){
                    user_input = trimwhitespace(user_input);
                    if(strlen(user_input) ==0){

                    }else{

                        if(!(strcmp(user_input,"end"))){    //end will stop user input process{
                            //count = count+1;
                            strcpy(memory.instruction_memory[count],"end");
                            count++;
                            break;
                        }
                        if(user_input[0] == '#') {
                            int i;
                            char* labelName = (char *)malloc((int)strlen(user_input)+1);

                            if(((int)strlen(user_input))>1){
                                user_input++;
                                bool spacefound= false;

                                for(i=0; i<((int)strlen(user_input));i++){

                                    //check for end of label string
                                    if(isspace((unsigned char)user_input[i])){

                                        spacefound=true;
                                        labelName[i] = '\0';  //add a null character
                                        user_input = user_input+i;
                                        user_input = trimwhitespace(user_input); //remove white space in user input
                                        break;
                                    }

                                    labelName[i]=user_input[i];
                                }

                                char* buf = (char*)malloc(5);
                                if(!spacefound){

                                    labelName[i+1] = '\0';
                                    sprintf(buf,"%d",count);
                                    jt_set(jumptable, labelName, buf);
                                    count--;
                                }
                                else{

                                    sprintf(buf,"%d",count);
                                    jt_set(jumptable, labelName, buf);

                                    if(!(strcmp(user_input,"end"))){    //end will stop user input process{

                                        strcpy(memory.instruction_memory[count],"end");
                                        count++;
                                        break;
                                    }

                                    strcpy(memory.instruction_memory[count],user_input);
                                }
                            }

                        }else{

                            strcpy(memory.instruction_memory[count],user_input);
                        }
                        count++;
                    }
                }
                else{

                }
            }
            if(count==MAX_INST){
                printf("\n oops! You have reached Maximum number of user instruction allowed\n");
            }

            fetch_Instr();

            printf("Final memory content :\n");
            printf("-------------------------------------------------------------------------------------------------------------------------\n");
            int i;
            for(i=500;i<506;i++){
                printf(" memory %d --> %d|", i, memory.data_memory[i]);
            }
            printf("\n-----------------------------------------------------------------------------------------------------------------------\n");

        }
        registr.ProgCounter = 0;
    }
}
