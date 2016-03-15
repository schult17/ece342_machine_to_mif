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

//instruction strings
const std::string MOVE = "mv";
const std::string MOVE_I = "mvi";
const std::string ADDITION = "add";
const std::string SUBTRACT = "sub";
const std::string LOAD = "ld";
const std::string STORE = "st";
const std::string MVNZ = "mvnz";
const std::string BREQ = "beq";

enum ErrorCode
{
    NO_ERROR,
    BAD_FILE,
    BAD_INSTR,
    BAD_REG,
    BAD_IMMED,
    BIG_IMMED,
    NEG_IMMED,
    BAD_INFILE,
    DEFINE_BAD,
    DEFINE_REDEF,
    LABEL_BAD,
    LABEL_REDEF,
    IMMED_LABEL_NF,
    WIDTH_DEPTH_ERROR,
    TO_MUCH_FOR_DEPTH,
    WIDTH_ERROR,
    DEPTH_ERROR,
    ONLY_LABEL      //internal error, not an actual error
};

//number of registers in the processor (r0-r7)
#define REG_COUNT 8

//stages
#define PRE_PROCESS 0
#define PROCESS 1

//max register values for our processor (32 or 16 bit)
#define MAX_INT_16U 65535
#define MAX_INT_32U 0xFFFFFFFF

//number of HEX values to display 
#define TWO_BYTE_DISPLAY 4      //16 bits
#define FOUR_BYTE_DISPLAY 8     //32 bits

//to make defines
const std::string DEF_KEY = ".define";

//reverse mappings for reg numbers and instruction numbers to strings (note instruction list must match encoding)
static const char * const REG_NAMES_LIST[] = { "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7" };
static const char * const INSTRU_TO_STR_LIST[] = { "mv", "mvi", "add", "sub", "ld", "st", "mvnz", "beq" };

//Main assembling and parsing functions
int assemble( char *infile, char *outfile );
ErrorCode find_all_labels( std::string infile, int *line_number );
std::vector<int> parse_fin( std::string infile, int *width, int *depth, ErrorCode &error_code, int *line_number );
std::vector<int> parse_instruction( std::string instr, ErrorCode &error, int curr_instruction_num, int stage );
std::string parse_labelled_line( std::string instruc, int curr_instr_num, ErrorCode &error, int stage );

//writes MIF file
void write_to_file( std::string outfile, std::vector<int> instructions, int width, int depth );
std::string instruction_to_str_comment( int instr, int next_instr );

//Parsing helper functions
void look_at_outputfilename( std::string &str );
ErrorCode look_at_inputfilename( std::string &str );
void remove_pre_space( std::string &str );
void remove_post_space( std::string &str );
int is_comment( std::string str );
int is_all_space( std::string i );
int is_instruction_mvi_or_beq( int instr );

//String to encoding/reg number functions
int parse_i( std::string i, ErrorCode &error );
int parse_reg( std::string rx, ErrorCode &error );
int parse_immediate( std::string imm, ErrorCode &error );

//creates the mif instruction
int mask_mif_instr( int i, int rx, int ry );
ErrorCode parse_define( std::string line );
std::string to_lower( std::string str );

//error handler
void print_error_message( ErrorCode code, int line, int depth, int instruction_count );

#endif