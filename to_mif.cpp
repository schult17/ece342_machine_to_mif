#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <stdlib.h>
#include <iomanip>
#include <ctype.h>
#include "to_mif.h"

using namespace std;

int assemble( char *infile, char *outfile )
{
    string fin = infile;
    string fout = ( outfile != NULL ) ? outfile : "the_mif.mif";
    
    look_at_outputfilename( fout );
    
    int width, depth, error, line;
    
    error = look_at_inputfilename( fin );
    
    if( error == NO_ERROR )
    {
        vector<int> instructions = parse_fin( fin, &width, &depth, &error, &line );
    
        if( error == NO_ERROR )
            write_to_file( fout, instructions, width, depth );
        else
            print_error_message( error, line );
    }
    else
    {
        print_error_message( error, 0 );
    }
    
    return 0;
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
    size_t pos_depth, pos_width;
    
    while( getline( file, instr ) )
    {
        if( instr != "" && instr != "\n" && !is_comment( instr ) )  //ignore new lines and comments
        {
            pos_width = instr.find( "WIDTH" );
            if( pos_width == string::npos ) pos_width = instr.find( "width" );
        
            pos_depth = instr.find( "DEPTH" );
            if( pos_depth == string::npos ) pos_depth = instr.find( "depth" );
        
            if( pos_width != string::npos )
            {
                instr.erase( 0, pos_width + 5 );
                *width = atoi( instr.c_str() );
            }
            else if( pos_depth != string::npos)
            {
                instr.erase( 0, pos_depth + 5 );
                *depth = atoi( instr.c_str() );
            }
            else
            {
                add = parse_instruction( instr, &error );
            
                if( error == NO_ERROR )
                {
                    mif.push_back( add[0] );
                    if( add.size() == 2 )   mif.push_back( add[1] );    //mvi
                }
                else
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

std::vector<int> parse_instruction( std::string instr, int *error )
{
    vector<int> ret;
    int mif_instr = -1;
    
    string in_s = "", rx_s = "", ry_s = "";
    
    remove_pre_space( instr );
    
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
    else if( error_imm != NO_ERROR )
        *error = error_imm;
    else
        mif_instr = mask_mif_instr( instruction, rx, ry );
    
    ret.push_back( mif_instr );
    
    if( instruction == MVI )    //move instruction needs 2 mif instructions
        ret.push_back( imm );
    
    return ret;
}

int parse_i( std::string i, int *error )
{
    int ret = -1;
    
    i = to_lower( i );  //case insensitive instructions
    
    *error = NO_ERROR;
    
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

string to_lower( string str )
{
    string str_lower;
    for( int i = 0; i < str.length(); i++ )
        str_lower.push_back( tolower( str[i] ) );
    
    return str_lower;
}

int parse_immediate( std::string imm, int *error )
{
    char *endptr;
    int num = strtol( imm.c_str(), &endptr, 0 );
    
    if( *endptr != '\0')
    {
        *error = BAD_IMMED;
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

int mask_mif_instr( int i, int rx, int ry )
{
    return ry | ( rx << 3 ) | ( i << 6 );
}

void write_to_file( string outfile, std::vector<int> instructions, int width, int depth )
{
    ofstream file( outfile, ofstream::out );
    
    //make sure file exists
    if( file.fail() )
    {
        cout << "ERROR: output file could not be opened: Sorry...." << endl;
        return;
    }
    
    file << "%For use in ECE342, Lab6%\n";
    file << "%Disclaimer: I tried%\n\n";
    file << "DEPTH = " << depth << ";\n";
    file << "WIDTH = " << width << ";\n";
    file << "ADDRESS_RADIX = HEX;\n";
    file << "DATA_RADIX = HEX; %using hex to be able to debug easier%\n\n";
    
    file << "CONTENT\n";
    file << "BEGIN\n";
    
    for( int i = 0; i < instructions.size(); i++ )
        file << i << " : " << hex << setw(4) << setfill('0') << uppercase << instructions[i] << ";\n";
    
    file << "END;\n";
    
    cout << "CLEAN FINISH: output file written to: \"" << outfile << "\"" << endl;
    
    file.close();
}

void look_at_outputfilename( string &str )
{
    if( str.find( ".mif" ) == string::npos )
    {
        str.push_back( '.' );
        str.push_back( 'm' );
        str.push_back( 'i' );
        str.push_back( 'f' );
    }
}

int look_at_inputfilename( string &str )
{
    if( str.find( ".tys" ) == string::npos )
        return BAD_INFILE;
    else
        return NO_ERROR;
}

void remove_pre_space( std::string &str )
{
    int i = 0;
    while( i < str.length() )
    {
        if( str[i] == ' ' || str[i] == '\t' )
            str.erase( i, 1 );
        else
            break;
        
        i++;
    }
}

int is_comment( string str )
{
    remove_pre_space( str );    //remove leading spaces
    
    //if first character after removing leading spaces is a #, its comment, ignore
    return ( str[0] == '#' ) ? 1 : 0;
}

void print_error_message( int code, int line )
{
    if( code == BAD_FILE )
        cout << "ERROR BAD INPUT FILE: could not locate input file" << endl;
    else if( code == BAD_INFILE )
        cout << "ERROR BAD INPUT FILE: input file must have '.tys' extension, my initials :|" << endl;
    else if( code == BAD_INSTR )
        cout << "ERROR: line " << line << ": unknown instruction" << endl;
    else if( code == BAD_REG )
        cout << "ERROR: line " << line << ": unknown register" << endl;
    else if( code == BAD_IMMED )
        cout << "ERROR: line " << line << ": the immediate value could not be interpreted as an integer" << endl;
    else if( code == BIG_IMMED )
        cout << "ERROR: line " << line << ": the immediate value is too large, it must be in the range [0, 65535]" << endl;
    else if( code == NEG_IMMED )
        cout << "ERROR: line " << line << ": the immediate value must be unsigned in the range [0, 65535]" << endl;
    else
        cout << "ERROR: Unknown, I missed it :(" << endl;
}
