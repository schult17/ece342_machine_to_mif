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

int assemble( char *infile, char *outfile )
{
    string fin = infile;
    string fout = ( outfile != NULL ) ? outfile : "the_mif.mif";
    
    look_at_outputfilename( fout );
    
    int width, depth, error, line;
    
    error = look_at_inputfilename( fin );
    
    if( error == NO_ERROR )
    {
        //first find all the labels
        error = find_all_labels( fin, &line );
        
        if( error != NO_ERROR )
        {
            print_error_message( error, line );
        }
        else
        {
            //if preprocess was good, parse the instructions
            vector<int> instructions = parse_fin( fin, &width, &depth, &error, &line );
    
            //if process was good, write mif file
            if( error == NO_ERROR )
                write_to_file( fout, instructions, width, depth );
            else
                print_error_message( error, line );
        }
    }
    else
    {
        print_error_message( error, 0 );
    }
    
    return 0;
}

//find all labels and defines FIRST, inefficient, but whatever
int find_all_labels( std::string infile, int *line_number )
{
    ifstream file( infile );
    
    //make sure file exists
    if( file.fail() )
        return BAD_FILE;
    
    string instr;
    int error;
    int line = 0, instruction_num = 0;
    vector<int> add;
    size_t pos_depth, pos_width, pos_def;
    
    while( getline( file, instr ) )
    {
        if( instr != "" && instr != "\n" && !is_comment( instr ) )  //ignore new lines and comments
        {
            pos_width = instr.find( "WIDTH" );
            if( pos_width == string::npos ) pos_width = instr.find( "width" );
            
            pos_depth = instr.find( "DEPTH" );
            if( pos_depth == string::npos ) pos_depth = instr.find( "depth" );
            
            pos_def = instr.find( DEF_KEY );
            
            //finding labels and defines
            if( pos_def != string::npos )
            {
                error = parse_define( instr );
                
                if( error != NO_ERROR ) //got error on define
                    break;
            }
            else if( !( pos_width != string::npos || pos_depth != string::npos ) )   //ignore depth and width in pre process
            {
                add = parse_instruction( instr, &error, instruction_num - 1, PRE_PROCESS );
                
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

vector<int> parse_fin( string infile, int *width, int *depth, int *error_code, int *line_number )
{
    //default width and depth
    *width = 16;
    *depth = 128;
    
    vector<int> mif;
    ifstream file( infile );
    
    //make sure file exists
    if( file.fail() )
    {
        *error_code = BAD_FILE;
        return mif;
    }
    
    string instr;
    int error;
    int line = 0;
    vector<int> add;
    size_t pos_depth, pos_width, pos_def;
    
    while( getline( file, instr ) )
    {
        if( instr != "" && instr != "\n" && !is_comment( instr ) )  //ignore new lines and comments
        {
            pos_width = instr.find( "WIDTH" );
            if( pos_width == string::npos ) pos_width = instr.find( "width" );
        
            pos_depth = instr.find( "DEPTH" );
            if( pos_depth == string::npos ) pos_depth = instr.find( "depth" );
            
            pos_def = instr.find( DEF_KEY );
        
            if( pos_width != string::npos )
            {
                instr.erase( 0, pos_width + 5 );
                
                char *endptr;
                int num = strtol( instr.c_str(), &endptr, 0 );
                
                if( *endptr != '\0' )
                {
                    *error_code = WIDTH_DEPTH_ERROR;
                    *line_number = line;
                    file.close();
                    return mif;
                }
                else
                {
                    *width = num;
                }
            }
            else if( pos_depth != string::npos)
            {
                instr.erase( 0, pos_depth + 5 );
                
                char *endptr;
                int num = strtol( instr.c_str(), &endptr, 0 );
                
                if( *endptr != '\0' )
                {
                    *error_code = WIDTH_DEPTH_ERROR;
                    *line_number = line;
                    file.close();
                    return mif;
                }
                else
                {
                    *depth = num;
                }
            }
            else if( pos_def != string::npos )
            {
                //do nothing, already handled this in PRE_PROCESS
            }
            else
            {
                add = parse_instruction( instr, &error, mif.size() - 1, PROCESS );
            
                if( error == NO_ERROR )
                {
                    mif.push_back( add[0] );
                    if( add.size() == 2 )   mif.push_back( add[1] );    //mvi
                }
                else if( error != ONLY_LABEL )  //skip to next line if this line was only a label
                {
                    *error_code = error;
                    *line_number = line;
                    file.close();
                    return mif;
                }
            }
        }
        
        line += 1;  //increment line even on blank lines
    }
    
    *line_number = line;
    *error_code = NO_ERROR;
    file.close();
    return mif;
}

std::vector<int> parse_instruction( std::string instr, int *error, int curr_instruction_num, int stage )
{
    vector<int> ret;
    int mif_instr = -1;
    
    string in_s = "", rx_s = "", ry_s = "";
    
    remove_pre_space( instr );
    
    int error_label = NO_ERROR;
    
    if( instr.find( ":" ) != string::npos )
    {
        instr = parse_labelled_line( instr, curr_instruction_num, &error_label, stage );
        
        if( error_label != NO_ERROR )
        {
            *error = error_label;
            return ret;
        }
        else if( is_all_space( instr ) )
        {
            *error = ONLY_LABEL;
            return ret;
        }
        
        remove_pre_space( instr );  //do this again after label
    }
    
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
                default:
                    break;
            }
        }
        
        i = i + 1;
    }
    
    //determining if instruction is valid
    int error_i = NO_ERROR, error_reg_x = NO_ERROR, error_reg_y = NO_ERROR, error_imm = NO_ERROR;
    int instruction = 0, rx = 0, ry = 0, imm = 0;
    
    instruction = parse_i( in_s, &error_i );
    rx = parse_reg( rx_s, &error_reg_x );
    
    if( instruction != MVI )
        ry = parse_reg( ry_s, &error_reg_y );
    else
        imm = parse_immediate( ry_s, &error_imm );
    
    *error = NO_ERROR;
    if( error_i != NO_ERROR )
        *error = error_i;
    else if( error_reg_x != NO_ERROR )
        *error = error_reg_x;
    else if( error_reg_y != NO_ERROR )
        *error = error_reg_y;
    else if( error_imm != NO_ERROR && stage != PRE_PROCESS )    //pre process, ignore this error
        *error = error_imm;
    else
        mif_instr = mask_mif_instr( instruction, rx, ry );
    //---------------------------------------------------//
    
    ret.push_back( mif_instr );
    
    if( instruction == MVI )    //move instruction needs 2 mif instructions
        ret.push_back( imm );
    
    return ret;
}

std::string parse_labelled_line( std::string instr, int curr_instr_num, int *error, int stage )
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
            *error = LABEL_BAD;
        else if( label_def_to_line_num.find( instr ) != label_def_to_line_num.end() )
            *error = LABEL_REDEF;
        else
            *error = NO_ERROR;
        
        label_def_to_line_num[instr] = curr_instr_num + 1;  //label is for next instruction (current one being parsed)
    }
    else
    {
        //if we are in PROCESS stage, we know its a valid label, so skip, but still return parsed instruction
        *error = NO_ERROR;
    }
    
    return ret;
}

//parse instruction (bits 8:6 in encoding)
int parse_i( std::string i, int *error )
{
    int ret = -1;
    
    i = to_lower( i );  //case insensitive instructions
    
    *error = NO_ERROR;
    
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
    else
        *error = BAD_INSTR;
    
    return ret;
}

//parse x register (bits 5:3 in encoding), and y register (bits 2:0 in encoding)
int parse_reg( std::string reg, int *error )
{
    *error = NO_ERROR;
    
    for( int i = 0; i < REG_COUNT; i++ )
    {
        if( to_lower( reg ) == REG_NAMES_L[i] )   //lower or uppercase
            return i;
    }
    
    *error = BAD_REG;
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
int parse_immediate( std::string imm, int *error )
{
    char *endptr;
    int num = strtol( imm.c_str(), &endptr, 0 );
    
    if( *endptr != '\0')
    {
        //couldn't parse the entire string (most likely not a number),
        //check for label or define
        if( label_def_to_line_num.find( imm ) == label_def_to_line_num.end() )
            *error = IMMED_LABEL_NF;
        else
            num = label_def_to_line_num[imm];
    }
    else
    {
        if( num > MAX_INT_16U )
            *error = BIG_IMMED;
        else if( num < 0 )
            *error = NEG_IMMED;
        else
            *error = NO_ERROR;
    }
    
    return num;
}

//parse .define statements, only accepts int inputs, no labels or other defines
int parse_define( std::string line )
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
        if( num > MAX_INT_16U )
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
    file << "%For use in ECE342, Lab6%\n";
    file << "%Disclaimer: I tried%\n\n";
    file << "WIDTH = " << width << ";\n";
    file << "DEPTH = " << depth << ";\n";
    file << "ADDRESS_RADIX = HEX;\n";
    file << "DATA_RADIX = HEX;\n\n";
    
    file << "CONTENT\n";
    file << "BEGIN\n";
    
    for( int i = 0; i < instructions.size(); i++ )
        file << i << " : " << hex << setw(4) << setfill('0') << uppercase << instructions[i] << ";\n";
    
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
int look_at_inputfilename( string &str )
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

//takes an error code and line number and prints the error message to STDOUT
void print_error_message( int code, int line )
{
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
            cout << "ERROR: line " << line << ": the immediate value is too large, it must be in the range [0, 65535]" << endl;
            break;
        case NEG_IMMED:
            cout << "ERROR: line " << line << ": the immediate value must be unsigned in the range [0, 65535]" << endl;
            break;
        case DEFINE_BAD:
            cout << "ERROR: line " << line << ": bad define statement, must 16 bit unsigned int (possible problem)" << endl;
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
        default:
            cout << "ERROR: Unknown, I missed it :(" << endl;
            break;
    }
    
    cout << "MIF-IFY FAILED " << code << endl;
}
