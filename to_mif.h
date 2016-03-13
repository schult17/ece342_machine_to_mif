#ifndef TO_MIF_H
#define TO_MIF_H

#include <iostream>
#include <vector>

//instruction masks, change to match your encodings possibly
#define MV 0
#define MVI 1
#define ADD 2
#define SUB 3
#define LD 4
#define ST 5
#define MZ 6
#define BEQ 7

//instructions
const std::string MOVE = "mv";
const std::string MOVE_I = "mvi";
const std::string ADDITION = "add";
const std::string SUBTRACT = "sub";
const std::string LOAD = "ld";
const std::string STORE = "st";
const std::string MVNZ = "mvnz";
const std::string BREQ = "beq";

//error codes
#define NO_ERROR 0
#define BAD_FILE 1
#define BAD_INSTR 2
#define BAD_REG 3
#define BAD_IMMED 4
#define BIG_IMMED 5
#define NEG_IMMED 6
#define BAD_INFILE 7
#define DEFINE_BAD 8
#define DEFINE_REDEF 9
#define LABEL_BAD 10
#define LABEL_REDEF 11
#define IMMED_LABEL_NF 12
#define WIDTH_DEPTH_ERROR 13
#define TO_MUCH_FOR_DEPTH 14

//error signals (not errors)
#define ONLY_LABEL 11

//stages
#define PRE_PROCESS 0
#define PROCESS 1

#define REG_COUNT 8
#define MAX_INT_16U 65535

//to make defines
const std::string DEF_KEY = ".define";

static const char * const REG_NAMES_L[] = { "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7" };
static const char * const instr_to_str[] = { "mv", "mvi", "add", "sub", "ld", "st", "mvnz", "beq" };
static const char * const reg_num_to_reg_name[] = { "r0", "r1", "r2", "r3", "r4", "r5", "r7", "r7" };

int assemble( char *infile, char *outfile );
int find_all_labels( std::string infile, int *line_number );
std::vector<int> parse_fin( std::string infile, int *width, int *depth, int *error_code, int *line_number );
std::vector<int> parse_instruction( std::string instr, int *error, int curr_instruction_num, int stage );
std::string parse_labelled_line( std::string instruc, int curr_instr_num, int *error, int stage );

void look_at_outputfilename( std::string &str );
int look_at_inputfilename( std::string &str );
void remove_pre_space( std::string &str );
void remove_post_space( std::string &str );
int is_comment( std::string str );

int parse_i( std::string i, int *error );
int parse_reg( std::string rx, int *error );
int parse_immediate( std::string imm, int *error );
int mask_mif_instr( int i, int rx, int ry );
int parse_define( std::string line );
std::string to_lower( std::string str );
int is_all_space( std::string i );
int is_instruction_mvi_or_beq( int instr );

std::string instruction_to_str_comment( int instr, int next_instr );

void write_to_file( std::string outfile, std::vector<int> instructions, int width, int depth );

void print_error_message( int code, int line, int depth, int instruction_count );

#endif