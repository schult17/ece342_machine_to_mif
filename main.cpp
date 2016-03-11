#include <stdio.h>
#include <iostream>
#include "to_mif.h"

using namespace std;

int main( int argc, char *argv[] )
{
    int code = -1;
    
    if( argc > 3 )
        cout << "Too many arguments, format: ./miffer input_file_name output_file_name [default a.mif]" << endl;
    else if( argc <= 1 )
        cout << "Not enough arguments, format: ./miffer input_file_name output_file_name [default a.mif]" << endl;
    else
        code = ( argc == 2 ) ? assemble( argv[1], NULL ) : assemble( argv[1], argv[2] );
            
    return 0;
}

