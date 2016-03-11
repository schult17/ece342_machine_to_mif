#ifndef TO_MIF_H
#define TO_MIF_H

#include <iostream>
#include <vector>

//instruction masks
#define MV 0
#define MVI 1
#define ADD 2
#define SUB 3
#define LD 4
#define ST 5
#define MZ 6

//error codes
#define NO_ERROR 0
#define BAD_FILE 1
#define BAD_INSTR 2
#define BAD_REG 3
#define BAD_IMMED 4
#define BIG_IMMED 5
#define NEG_IMMED 6
#define BAD_INFILE 7

#define REG_COUNT 8
#define MAX_INT_16U 65535

//instructions
const std::string MOVE = "mv";
const std::string MOVE_I = "mvi";
const std::string ADDITION = "add";
const std::string SUBTRACT = "sub";
const std::string LOAD = "ld";
const std::string STORE = "st";
const std::string MVNZ = "mvnz";

static const char * const REG_NAMES_L[] = { "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7" };

int assemble( char *infile, char *outfile );
std::vector<int> parse_fin( std::string infile, int *width, int *depth, int *error_code, int *line_number );
std::vector<int> parse_instruction( std::string instr, int *error );

void look_at_outputfilename( std::string &str );
int look_at_inputfilename( std::string &str );
void remove_pre_space( std::string &str );
int is_comment( std::string str );

int parse_i( std::string i, int *error );
int parse_reg( std::string rx, int *error );
int parse_immediate( std::string imm, int *error );
int mask_mif_instr( int i, int rx, int ry );
std::string to_lower( std::string str );

void write_to_file( std::string outfile, std::vector<int> instructions, int width, int depth );

void print_error_message( int code, int line );

#endif