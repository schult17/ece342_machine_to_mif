#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdlib.h>
#include <iomanip>
#include <ctype.h>
#include <unordered_map>
#include "to_mif.h"

using namespace std;

//define and label map
unordered_map<string, int> label_def_to_line_num;

//global width for build
int width_bits = 16;
int depth_bytes = 128;

int assemble( char *infile, char *outfile )
{
    string fin = infile;
    string fout = ( outfile != NULL ) ? outfile : "the_mif.mif";
    
    look_at_outputfilename( fout );
    
    int width, depth, line;
    ErrorCode error;
    
    error = look_at_inputfilename( fin );
    
    if( error == NO_ERROR )
    {
        //first find all the labels
        error = find_all_labels( fin, &line, &width, &depth );
        
        if( error != NO_ERROR )
        {
            print_error_message( error, line, -1, -1 );
        }
        else
        {
            //if preprocess was good, parse the instructions
            vector<int> instructions = parse_fin( fin, error, &line );
    
            //if process was good, write mif file
            if( error == NO_ERROR )
                write_to_file( fout, instructions, width, depth );
            else
                print_error_message( error, line, depth, instructions.size() );
        }
    }
    else    //only file errors here
    {
        print_error_message( error, 0, -1, -1 );
    }
    
    return 0;
}

//find all labels and defines FIRST (as well as WIDTH/DEPTH flag), inefficient, but whatever
ErrorCode find_all_labels( std::string infile, int *line_number, int *width, int *depth)
{
    ifstream file( infile );
    
    //make sure file exists
    if( file.fail() )
        return BAD_FILE;
    
    //default width and depth
    *width = width_bits = 16;
    *depth = depth_bytes = 128;
    
    string instr;
    ErrorCode error;
    int line = 0, instruction_num = 0;
    vector<int> add;
    size_t pos_depth = 0, pos_width = 0, pos_def = 0, pos_hashtag = 0;
    
    bool lined_comment = false;
    
    while( getline( file, instr ) )
    {
        if( instr != "" && instr != "\n" && !is_comment( instr ) )  //ignore new lines and comments
        {
            pos_width = instr.find( "WIDTH" );
            
            pos_depth = instr.find( "DEPTH" );
            
            pos_def = instr.find( DEF_KEY );
            
            pos_hashtag = instr.find( "#" );    //will ignore width, depth, define behind a comment
            
            //finding labels and defines
            if( pos_width != string::npos )
            {
                if( pos_hashtag > pos_width )   //ignore if its in a comment
                {
                    instr.erase( 0, pos_width + 5 );
                    
                    char *endptr;
                    int num = strtol( instr.c_str(), &endptr, 0 );
                    
                    if( *endptr != '\0' )
                    {
                        error = WIDTH_DEPTH_ERROR;
                        *line_number = line;
                        file.close();
                        return error;
                    }
                    else if( pos_def != string::npos )
                    {
                        error = WIDTH_DEPTH_DEFINE;
                        *line_number = line;
                        file.close();
                        return error;
                    }
                    else
                    {
                        if( num != 32 && num != 16 )
                        {
                            error = WIDTH_ERROR;
                            *line_number = line;
                            file.close();
                            return error;
                        }
                        else
                        {
                            *width = num;
                            width_bits = num;
                        }
                    }
                }
            }
            else if( pos_depth != string::npos)
            {
                if( pos_hashtag > pos_depth )
                {
                    instr.erase( 0, pos_depth + 5 );
                    
                    char *endptr;
                    int num = strtol( instr.c_str(), &endptr, 0 );
                    
                    if( *endptr != '\0' )
                    {
                        cout << instr << endl;
                        error = WIDTH_DEPTH_ERROR;
                        *line_number = line;
                        file.close();
                        return error;
                    }
                    else if( pos_def != string::npos )
                    {
                        error = WIDTH_DEPTH_DEFINE;
                        *line_number = line;
                        file.close();
                        return error;
                    }
                    else
                    {
                        if( num % 2 )
                        {
                            error = DEPTH_ERROR;
                            *line_number = line;
                            file.close();
                            return error;
                        }
                        else
                        {
                            *depth = num;
                            depth_bytes = num;
                        }
                    }
                }
            }
            else if( pos_def != string::npos )
            {
                if( pos_hashtag > pos_def )
                {
                    error = parse_define( instr );
                    
                    if( error != NO_ERROR ) //got error on define
                        break;
                }
            }
            else
            {
                add = parse_instruction( instr, error, instruction_num - 1, PRE_PROCESS );
                
                if( error == NO_ERROR )
                    instruction_num += add.size();
                else if( error != ONLY_LABEL )  //skip to next line if this line was only a label
                    break;
            }
        }
        
        line += 1;  //increment line even on blank lines
    }
    
    *line_number = line;
    file.close();
    
    return error;
}

vector<int> parse_fin( string infile, ErrorCode &error_code, int *line_number )
{
    vector<int> mif;
    ifstream file( infile );
    
    //make sure file exists
    if( file.fail() )
    {
        error_code = BAD_FILE;
        return mif;
    }
    
    string instr;
    ErrorCode error;
    int line = 0;
    vector<int> add;
    size_t pos_depth = 0, pos_width = 0, pos_def = 0, pos_hashtag = 0;
    
    bool found_behind_hash_tag = false;
    
    while( getline( file, instr ) )
    {
        if( instr != "" && instr != "\n" && !is_comment( instr ) )  //ignore new lines and comments
        {
            pos_width = instr.find( "WIDTH" );
        
            pos_depth = instr.find( "DEPTH" );
            
            pos_def = instr.find( DEF_KEY );
            
            pos_hashtag = instr.find( "#" );
            
            found_behind_hash_tag = false;
            if( pos_width > pos_hashtag )   found_behind_hash_tag = true;
            if( pos_depth > pos_hashtag )   found_behind_hash_tag = true;
            if( pos_def > pos_hashtag )     found_behind_hash_tag = true;
        
            //all these cases handled in pre processing
            if( !( pos_width != string::npos || pos_depth != string::npos || pos_def != string::npos || found_behind_hash_tag ) )
            {
                add = parse_instruction( instr, error, mif.size() - 1, PROCESS );
            
                if( error == NO_ERROR )
                {
                    mif.push_back( add[0] );
                    if( add.size() == 2 )   mif.push_back( add[1] );    //mvi or beq
                }
                else if( error != ONLY_LABEL )  //skip to next line if this line was only a label
                {
                    error_code = error;
                    *line_number = line;
                    file.close();
                    return mif;
                }
            }
        }
        
        line += 1;  //increment line even on blank lines
    }
    
    //if we get here, we have no gotten an error yet, check if code takes up too much memory
    error_code = ( mif.size() > depth_bytes ) ? TO_MUCH_FOR_DEPTH : NO_ERROR;
    *line_number = line;
    file.close();
    return mif;
}

std::vector<int> parse_instruction( std::string instr, ErrorCode &error, int curr_instruction_num, int stage )
{
    vector<int> ret;
    int mif_instr = 0;
    
    string in_s = "", rx_s = "", ry_s = "", imm_s = "";
    
    remove_pre_space( instr );
    
    ErrorCode error_label = NO_ERROR;
    
    if( instr.find( ":" ) != string::npos )
    {
        instr = parse_labelled_line( instr, curr_instruction_num, error_label, stage );
        
        if( error_label != NO_ERROR )
        {
            error = error_label;
            return ret;
        }
        else if( is_all_space( instr ) )
        {
            error = ONLY_LABEL;
            return ret;
        }
        
        remove_pre_space( instr );  //do this again after label
    }
    
    //parsing out the instruction, rx, ry and or immediate value
    int i = 0;
    int entry = 0;
    while( i < instr.length() )
    {
        if( instr[i] == ' ' || instr[i] == '\t' )   //tabs or spaces
        {
            entry += 1;
        }
        else
        {
            switch( entry )
            {
                case 0:
                    in_s.push_back( instr[i] );
                    break;
                case 1:
                    rx_s.push_back( instr[i] );
                    break;
                case 2:
                    ry_s.push_back( instr[i] );
                    break;
                case 3: //only for beq
                    imm_s.push_back( instr[i] );
                    break;
                default:
                    break;
            }
        }
        
        i = i + 1;
    }
    
    //determining if instruction is valid
    ErrorCode error_i = NO_ERROR, error_reg_x = NO_ERROR, error_reg_y = NO_ERROR, error_imm = NO_ERROR;
    int instruction = 0, rx = 0, ry = 0, imm = 0;
    
    instruction = parse_i( in_s, error_i );
    rx = parse_reg( rx_s, error_reg_x );
    
    if( instruction == MVI )
        imm = parse_immediate( ry_s, error_imm );
    else if( instruction == BEQ  )
        imm = parse_immediate( imm_s, error_imm );
    else
        ry = parse_reg( ry_s, error_reg_y );
    
    error = NO_ERROR;
    if( error_i != NO_ERROR )
        error = error_i;
    else if( error_reg_x != NO_ERROR )
        error = error_reg_x;
    else if( error_reg_y != NO_ERROR )
        error = error_reg_y;
    else if( error_imm != NO_ERROR && stage != PRE_PROCESS )    //pre process, ignore this error
        error = error_imm;
    else
        mif_instr = mask_mif_instr( instruction, rx, ry );
    //---------------------------------------------------//
    
    //add the instruction
    ret.push_back( mif_instr );
    
    //move immediate and beq instruction needs 2 mif instructions
    //second is the immediate value
    if( instruction == MVI || instruction == BEQ )
        ret.push_back( imm );
    
    return ret;
}

std::string parse_labelled_line( std::string instr, int curr_instr_num, ErrorCode &error, int stage )
{
    size_t pos_col = instr.find( ":" );
    
    string ret = instr;
    
    //no clue why its pos_col + 1, should be just pos_col by what the C++ docs say...
    ret.erase( 0, pos_col + 1 );
    instr.erase( pos_col + 1, string::npos );
    
    remove_post_space( instr );
    
    if( stage == PRE_PROCESS )
    {
        if( instr.find( " " ) != string::npos )
            error = LABEL_BAD;
        else if( label_def_to_line_num.find( instr ) != label_def_to_line_num.end() )
            error = LABEL_REDEF;
        else
            error = NO_ERROR;
        
        label_def_to_line_num[instr] = curr_instr_num + 1;  //label is for next instruction (current one being parsed)
    }
    else
    {
        //if we are in PROCESS stage, we know its a valid label, so skip, but still return parsed instruction
        error = NO_ERROR;
    }
    
    return ret;
}

//parse instruction (bits 8:6 in encoding)
int parse_i( std::string i, ErrorCode &error )
{
    int ret = 0;
    
    i = to_lower( i );  //case insensitive instructions
    
    error = NO_ERROR;
    
    //if, else if blocks compare i to defined strings in .h file
    //if they match return the corresponding instruction number
    if( i == MOVE )
        ret = MV;
    else if( i == MOVE_I )
        ret = MVI;
    else if( i == ADDITION )
        ret = ADD;
    else if( i == SUBTRACT )
        ret = SUB;
    else if( i == LOAD )
        ret = LD;
    else if( i == STORE )
        ret = ST;
    else if( i == MVNZ )
        ret = MZ;
    else if( i == BREQ )
        ret = BEQ;
    else
        error = BAD_INSTR;
    
    return ret;
}

//parse x register (bits 5:3 in encoding), and y register (bits 2:0 in encoding)
int parse_reg( std::string reg, ErrorCode &error )
{
    error = NO_ERROR;
    
    for( int i = 0; i < REG_COUNT; i++ )
    {
        if( to_lower( reg ) == REG_NAMES_LIST[i] )   //lower or uppercase
            return i;
    }
    
    error = BAD_REG;
    return -1;
}

//converts a string to lower case (for comparison)
string to_lower( string str )
{
    string str_lower;
    for( int i = 0; i < str.length(); i++ )
        str_lower.push_back( tolower( str[i] ) );
    
    return str_lower;
}

//parses the immediate value given to MVI
int parse_immediate( std::string imm, ErrorCode &error )
{
    char *endptr;
    int num = strtol( imm.c_str(), &endptr, 0 );
    
    if( *endptr != '\0')
    {
        //couldn't parse the entire string (most likely not a number),
        //check for label or define
        if( label_def_to_line_num.find( imm ) == label_def_to_line_num.end() )
            error = IMMED_LABEL_NF;
        else
            num = label_def_to_line_num[imm];
    }
    else
    {
        if( num > ( (width_bits == 16) ? MAX_INT_16U : MAX_INT_32U ) )
            error = BIG_IMMED;
        else if( num < 0 )
            error = NEG_IMMED;
        else
            error = NO_ERROR;
    }
    
    return num;
}

//parse .define statements, only accepts int inputs, no labels or other defines
ErrorCode parse_define( std::string line )
{
    size_t pos = line.find( ".define" );
    
    line.erase( 0, pos + 7 );   //erase the define
    remove_pre_space( line );
    
    //find label and value of define
    string label = "", val = "";
    
    int i = 0, s = 0;
    
    while( i < line.length() )
    {
        //when we hit a space, switch to value (from label) string
        if( line[i] == ' ' || line[i] == '\t'  )
            s += 1;
        else if( s == 0 )
            label.push_back( line[i] );
        else if( s== 1 )
            val.push_back( line[i] );
        
        i++;
    }
    
    //try to convert to a number
    char *endptr;
    int num = strtol( val.c_str(), &endptr, 0 );
    
    if( *endptr != '\0')
    {
        return DEFINE_BAD;
    }
    else
    {
        if( num > ( (width_bits == 16) ? MAX_INT_16U : MAX_INT_32U ) )
            return DEFINE_BAD;
        else if( num < 0 )
            return DEFINE_BAD;
    }
    
    if( label_def_to_line_num.find( label ) != label_def_to_line_num.end() )
        return DEFINE_REDEF;
    else
        label_def_to_line_num[label] = num;
    
    return NO_ERROR;
}

//creates a MIF number based on the instruction given
//format is DDDDDDDIIIXXXYYY where D is don't care
//I is instruction, X is x register, Y is y register
int mask_mif_instr( int i, int rx, int ry )
{
    return ry | ( rx << 3 ) | ( i << 6 );
}

//write to output file
void write_to_file( string outfile, std::vector<int> instructions, int width, int depth )
{
    ofstream file( outfile, ofstream::out );
    
    //make sure file exists
    if( file.fail() )
    {
        cout << "ERROR: output file could not be opened: Sorry...." << endl;
        return;
    }
    
    //whats is actually written to the output file
    //Some info in comments first
    file << "% For use in ECE342, Lab6 %\n";
    file << "% Disclaimer: I tried.... %\n\n";
    file << "%Assumed Encodings (note, beq instruction is optional!):%\n";
    file << "%\tmv = 3'b000\t%\n";
    file << "%\tmvi = 3'b001\t%\n";
    file << "%\tadd = 3'b010\t%\n";
    file << "%\tsub = 3'b011\t%\n";
    file << "%\tld = 3'b100\t%\n";
    file << "%\tst = 3'b101\t%\n";
    file << "%\tmvnz = 3'b110\t%\n";
    file << "%\tbeq = 3'b111\t%\n\n";
    file << "%Assumed positions of instruction and register in 16 bit input%\n";
    file << "%I = instruction, X = x register, Y = y register, D = don't care%\n";
    
    if( width_bits == 16 )
    {
        file << "% 16      8       0 %\n";
        file << "%  DDDDDDDIIIXXXYYY %\n";
    }
    else
    {
        file << "% 31              15      8        0 %\n";
        file << "%  DDDDDDDDDDDDDDDDDDDDDDDIIIXXXYYY  %\n";
    }
    
    file << "%\tLITTLE ENDIAN\t%\n\n";
    
    file << "%For parts 6 and 7 of the lab, open this MIF file in quartus%\n";
    file << "%and from the File menu select 'Save As' and save the file as a .hex file%\n";
    file << "%and Quartus will automatically convert it to a HEX file%\n\n";
    //---------------------------//
    
    //start writing file
    file << "WIDTH = " << width << ";\n";
    file << "DEPTH = " << depth << ";\n";
    file << "ADDRESS_RADIX = HEX;\n";
    file << "DATA_RADIX = HEX;\n\n";
    
    file << "CONTENT\n";
    file << "BEGIN\n";
    
    int last_instr_mvi = 0;
    int mvi_beq_num = 0;
    
    for( int i = 0; i < instructions.size(); i++ )
    {
        file << i << " : " << hex << setw( ( (width_bits == 16) ? TWO_BYTE_DISPLAY : FOUR_BYTE_DISPLAY ) ) << setfill('0') << uppercase << instructions[i];
        
        //cannot be last instructions if it is a mvi
        if( is_instruction_mvi_or_beq( instructions[i] ) )
        {
            if( i < instructions.size() - 1 )   //double sure
                mvi_beq_num = instructions[i + 1];
        }
        else
        {
            mvi_beq_num = -1;
        }
            
        
        //adding comment to line if this line is not second part of MVI or BEQ instruction
        if( last_instr_mvi )
            file << ";\n";
        else
            file << ";\t%" << instruction_to_str_comment( instructions[i], mvi_beq_num ) << "%\n";
        
        
        last_instr_mvi = is_instruction_mvi_or_beq( instructions[i] );
    }
    
    file << "END;\n";
    //---------------------------------------------//
    
    cout << "MIF-IFY SUCCESS: output file written to: \"" << outfile << "\"" << endl;
    
    file.close();
}

void look_at_outputfilename( string &str )
{
    //if output file name does not contain .mif, make sure it does
    //to be recognized by Quartus
    if( str.find( ".mif" ) == string::npos )
    {
        str.push_back( '.' );
        str.push_back( 'm' );
        str.push_back( 'i' );
        str.push_back( 'f' );
    }
}

//confirm input file
ErrorCode look_at_inputfilename( string &str )
{
    if( str.find( ".tys" ) == string::npos )
        return BAD_INFILE;
    else
        return NO_ERROR;
}

//removes all space prior to a string (strip forward)
void remove_pre_space( std::string &str )
{
    while( str.length() > 0 )
    {
        if( str[0] == ' ' || str[0] == '\t' )
            str.erase( 0, 1 );
        else
            break;
    }
}

//removes all space after first word until another word is
//encountered, if so, thats an error, quit (this is used for
//parsing labels and defines)
void remove_post_space( std::string &str )
{
    int i = 0;
    int space_seen = 0;
    int error = 0;
    while( i < str.length() )
    {
        if( space_seen )
        {
            if( str[i] == ' ' || str[i] == '\t' )
                str.erase( i, 1 );
            else
            {
                //space already seen and we get another letter!
                error = 1;
                break;
            }
        }
        else
        {
            if( str[i] == ' ' || str[i] == '\t' )
                space_seen = 1;
            
            i++;    //only increment non-spaces or tabs
        }
    }
    
    //erase first space we missed if this is a valid label
    if( str.length() > 0 && !error )
        str.erase( str.length() - 1, 1 );
}

//check if a line is a comment
int is_comment( string str )
{
    remove_pre_space( str );    //remove leading spaces
    
    //if first character after removing leading spaces is a #, its comment, ignore
    //two options for comments now ( # and %)
    return ( str[0] == '#' || str[0] == '%' ) ? 1 : 0;
}

//checks if a line is all white space or not
int is_all_space( std::string i )
{
    for( int p = 0; p < i.length(); p++ )
    {
        if( i[p] != ' ' && i[p] != '\t' )
            return 0;
    }
    
    return 1;
}

int is_instruction_mvi_or_beq( int instr )
{
    return ( instr >> 6 == MVI || instr >> 6 == BEQ );
}

std::string instruction_to_str_comment( int instr, int next_instr )
{
    //build comment for
    string ret = "";
    int rx, ry, i;
    
    ry = (instr & 0x0007);              //0000 0000 0000 0111
    rx = ( (instr >> 3 ) & 0x0007 );    //0000 0000 0011 1000
    i = ( (instr >> 6 ) & 0x0007 );     //0000 0001 1100 0000
    
    ret.append( INSTRU_TO_STR_LIST[i] );
    ret.append( " " );
    ret.append( REG_NAMES_LIST[rx] );
    
    ret.append( " " );
    
    ( i == MVI ) ? ret.append( std::to_string( next_instr ) ) : ret.append( REG_NAMES_LIST[ry] );
    
    return ret;
}

//takes an error code and line number and prints the error message to STDOUT
void print_error_message( ErrorCode code, int line, int depth, int instruction_count )
{
    unsigned int max_num = (width_bits == 16) ? MAX_INT_16U : MAX_INT_32U;
    switch ( code )
    {
        case BAD_FILE:
            cout << "ERROR BAD INPUT FILE: could not locate input file" << endl;
            break;
        case BAD_INFILE:
            cout << "ERROR BAD INPUT FILE: input file must have '.tys' extension, my initials :|" << endl;
            break;
        case BAD_INSTR:
            cout << "ERROR: line " << line << ": unknown instruction" << endl;
            break;
        case BAD_REG:
            cout << "ERROR: line " << line << ": unknown register" << endl;
            break;
        case BAD_IMMED:
            cout << "ERROR: line " << line << ": the immediate value could not be interpreted as an integer" << endl;
            break;
        case BIG_IMMED:
            cout << "ERROR: line " << line << ": the immediate value is too large, it must be in the range [0, " << max_num << "]" << endl;
            break;
        case NEG_IMMED:
            cout << "ERROR: line " << line << ": the immediate value must be unsigned in the range [0, " << max_num << "]" << endl;
            break;
        case DEFINE_BAD:
            cout << "ERROR: line " << line << ": bad define statement, must " << width_bits << " bit unsigned int (possible problem)" << endl;
            break;
        case DEFINE_REDEF:
            cout << "ERROR: line " << line << ": define redefinition" << endl;
            break;
        case LABEL_BAD:
            cout << "ERROR: line " << line << ": bad label" << endl;
            break;
        case LABEL_REDEF:
            cout << "ERROR: line " << line << ": label redefinition" << endl;
            break;
        case IMMED_LABEL_NF:
            cout << "ERROR: line " << line << ": undeclared identifier (label or define)" << endl;
            break;
        case WIDTH_DEPTH_ERROR:
            cout << "ERROR : line " << line << ": bad declaration of width or depth flag (possibly lose the equals sign?)" << endl;
            break;
        case TO_MUCH_FOR_DEPTH:
            cout << "ERROR MEMORY OVERFLOW: there are too many instructions for the depth of the memory: memory depth = " << depth << ", instruction count = " << instruction_count << "\n";
            break;
        case WIDTH_ERROR:
            cout << "ERROR: Only 32 and 16 bit data widths are supported" << endl;
            break;
        case DEPTH_ERROR:
            cout << "ERROR: Memory depth must be an integer multiple of 2" << endl;
            break;
        case WIDTH_DEPTH_DEFINE:
            cout << "ERROR: line " << line << ": symbols WIDTH and DEPTH are reserved, they cannot be defined using .define" << endl;
            break;
        default:
            cout << "ERROR: Unknown, I missed it :(" << endl;
            break;
    }
    
    cout << "MIF-IFY FAILED " << code << endl;
}
