/*============================================================================
MIT License

Copyright (c) 2023 Trevor Monk

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
============================================================================*/

/*!
 * @defgroup varcreate varcreate
 * @brief Command-Line utility for creating varserver variables from a file
 * @{
 */

/*==========================================================================*/
/*!
@file varcreate.c

    Variable Creation Utility

    The varcreate utility is a command-line tool for creating varserver
    variables from a JSON configuration file.

    This utility simply wraps the logic contained within the
    libvarcreate.so shared object.

    @see libvarcreate for more information
*/
/*==========================================================================*/

/*============================================================================
        Includes
============================================================================*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <varserver/varserver.h>
#include <varcreate/libvarcreate.h>

/*============================================================================
        Private definitions
============================================================================*/

/*============================================================================
        Private types
============================================================================*/

/*! state of the varcreate utility */
typedef struct _State
{
    /*! enable/disable verbose operation */
    bool verbose;

    /*! specifies the name of the variable file to load */
    char *filename;

    /*! instance identifier to apply to the variables in the file */
    uint32_t instanceID;

    /*! variable name prefix to apply to the variables in the file */
    char *prefix;

    /*! flags to apply to the variables in the file */
    char *flags;

} State;

/*============================================================================
        Private file scoped variables
============================================================================*/

/*============================================================================
        Private function declarations
============================================================================*/
static int ProcessOptions( int argc, char *argv[], State *pState );

/*============================================================================
        Public function definitions
============================================================================*/

/*==========================================================================*/
/*  main                                                                    */
/*!
    Main entry point for the variable creation utility

    The main function starts the variable creation utility

    @param[in]
        argc
            number of arguments on the command line
            (including the command itself)

    @param[in]
        argv
            array of pointers to the command line arguments

    @return none

============================================================================*/
int main(int argc, char **argv)
{
    State state;
    VarCreateOptions options = {0};
    VARSERVER_HANDLE hVarServer = NULL;
    int rc = 1;

    if( ProcessOptions( argc, argv, &state ) == EOK )
    {
        options.prefix = state.prefix;
        options.instanceID = state.instanceID;
        options.verbose = state.verbose;

        if ( state.flags != NULL )
        {
            (void)VARSERVER_StrToFlags( state.flags, &options.flags );
        }

        if( state.filename != NULL )
        {
            /* get a handle to the VAR server */
            hVarServer = VARSERVER_Open();
            if( hVarServer != NULL )
            {
                if ( state.verbose )
                {
                    printf("VARCREATE: Creating vars: %s\n", state.filename);
                }

                rc = VARCREATE_CreateFromFile( hVarServer,
                                               state.filename,
                                               &options );
                if( rc != EOK )
                {
                    fprintf( stderr,
                             "varcrate: error creating vars:%s\n",
                             strerror(rc));
                }
            }

            VARSERVER_Close( hVarServer );
        }
    }

    return rc;
}

/*==========================================================================*/
/*  ProcessOptions                                                          */
/*!
    Process the command line options for the varcreate utility

    The ProcessOptions function processes the command line arguments
    of the varcreate utility.

    Options include:

    -v : enable verbose output

    -i : apply an instance identifier to the variables

    -f : apply flags to the variables

    -p : apply a variable name prefix to the variables

    @param[in]
        argc
            number of arguments on the command line
            (including the command itself)

    @param[in]
        argv
            array of pointers to the command line arguments

    @param[in]
        pState
            pointer to the varcreate state structure to update

    @retval EOK - varcreate was successful
    @retval EINVAL - invalid arguments

============================================================================*/
static int ProcessOptions( int argc, char *argv[], State *pState )
{
    int result = EINVAL;
    int c;

    /* clear the State object */
    memset( pState, 0, sizeof( State ) );

    if( ( argc >= 2 ) && ( pState != NULL ) )
    {
        while( ( c = getopt( argc, argv, "vp:i:f:") ) != -1 )
        {
            switch( c )
            {
                case 'v':
                    pState->verbose = true;
                    break;

                case 'i':
                    pState->instanceID = atol( optarg );
                    break;

                case 'p':
                    pState->prefix = optarg;
                    break;

                case 'f':
                    pState->flags = optarg;
                    break;

                default:
                    break;
            }
        }

        /* get the name of the file to load */
        pState->filename = argv[argc-1];

        result = EOK;
    }

    return result;
}

