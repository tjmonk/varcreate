/*==============================================================================
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
==============================================================================*/

/*!
 * @defgroup varcreate varcreate
 * @brief Command-Line utility for creating varserver variables from a file
 * @{
 */

/*============================================================================*/
/*!
@file varcreate.c

    Variable Creation Utility

    The varcreate utility is a command-line tool for creating varserver
    variables from a JSON configuration file.

    This utility simply wraps the logic contained within the
    libvarcreate.so shared object.

    @see libvarcreate for more information
*/
/*============================================================================*/

/*==============================================================================
        Includes
==============================================================================*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <varserver/varserver.h>
#include <varcreate/libvarcreate.h>

/*==============================================================================
        Private definitions
==============================================================================*/

/*==============================================================================
        Private types
==============================================================================*/

/*! state of the varcreate utility */
typedef struct _State
{
    /*! enable/disable verbose operation */
    bool verbose;

    /*! specifies the name of the variable file or directory to load */
    char *name;

    /*! instance identifier to apply to the variables in the file */
    uint32_t instanceID;

    /*! variable name prefix to apply to the variables in the file */
    char *prefix;

    /*! flags to apply to the variables in the file */
    char *flags;

    /*! name represents a directory name */
    bool directory;

} State;

/*==============================================================================
        Private file scoped variables
==============================================================================*/

/*==============================================================================
        Private function declarations
==============================================================================*/
static int ProcessOptions( int argc, char *argv[], State *pState );
static int CreateFromDirectory( State *pState,
                                VARSERVER_HANDLE hVarServer,
                                char *dirname,
                                VarCreateOptions *pOptions );
static char *CreateFullPath( char *dirname, char *filename );

/*==============================================================================
        Public function definitions
==============================================================================*/

/*============================================================================*/
/*  main                                                                      */
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

==============================================================================*/
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

        if( state.name != NULL )
        {
            /* get a handle to the VAR server */
            hVarServer = VARSERVER_Open();
            if( hVarServer != NULL )
            {
                if ( state.directory == true )
                {
                    /* create variables from all the varcreate JSON files
                       in the directory */
                    rc = CreateFromDirectory( &state,
                                              hVarServer,
                                              state.name,
                                              &options );
                }
                else
                {
                    if ( state.verbose )
                    {
                        printf("VARCREATE: Creating vars: %s\n", state.name);
                    }

                    rc = VARCREATE_CreateFromFile( hVarServer,
                                                state.name,
                                                &options );
                }

                if( rc != EOK )
                {
                    fprintf( stderr,
                             "varcreate: error creating vars\n" );
                }
            }

            VARSERVER_Close( hVarServer );
        }
    }

    return rc == 0 ? 0 : 1;
}

/*============================================================================*/
/*  ProcessOptions                                                            */
/*!
    Process the command line options for the varcreate utility

    The ProcessOptions function processes the command line arguments
    of the varcreate utility.

    Options include:

    -v : enable verbose output

    -i : apply an instance identifier to the variables

    -f : apply flags to the variables

    -p : apply a variable name prefix to the variables

    -d : create from multiple files in a directory

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

==============================================================================*/
static int ProcessOptions( int argc, char *argv[], State *pState )
{
    int result = EINVAL;
    int c;

    /* clear the State object */
    memset( pState, 0, sizeof( State ) );

    if( ( argc >= 2 ) && ( pState != NULL ) )
    {
        while( ( c = getopt( argc, argv, "vp:i:f:d") ) != -1 )
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

                case 'd':
                    pState->directory = true;
                    break;

                default:
                    break;
            }
        }

        /* get the name of the file to load */
        pState->name = argv[argc-1];

        result = EOK;
    }

    return result;
}

/*============================================================================*/
/*  CreateFromDirectory                                                       */
/*!
    Create varserver variables from varcreate json files in a directory

    The CreateFromDirectory function opens the specified directory and
    iterates through all the *.json files in it and invokes
    VARCREATE_CreateFromFile on them.

    @param[in]
        hVarServer
            handle to the variable server

    @param[in]
        dirname
            name of the directory to process

    @param[in]
        pOptions
            pointer to the VarCreateOptions to apply to each variable

    @retval EOK - varcreate was successful
    @retval EINVAL - invalid arguments

==============================================================================*/
static int CreateFromDirectory( State *pState,
                                VARSERVER_HANDLE hVarServer,
                                char *dirname,
                                VarCreateOptions *pOptions )
{
    int result = EINVAL;
    int rc;
    struct dirent *dp;
    DIR *dfd;
    struct stat st;
    char *pFilename;
    char *p;

    if ( ( dirname != NULL ) &&
         ( pOptions != NULL ) &&
         ( pState != NULL ) )
    {
        /* assume everything is ok until it is not */
        result = EOK;

        dfd = opendir(dirname);
        if ( dfd != NULL )
        {
            /* iterate through each entry in the directory */
            while ( ( dp = readdir(dfd) ) != NULL )
            {
                pFilename = CreateFullPath( dirname, dp->d_name );
                if ( pFilename == NULL )
                {
                    continue;
                }

                rc = stat( pFilename, &st );
                if ( rc == -1 )
                {
                    fprintf( stderr, "Unable to stat file: %s\n", pFilename );
                    continue;
                }

                if ( ( st.st_mode & S_IFMT ) == S_IFDIR )
                {
                    /* skip directories */
                    continue;
                }

                p = strstr( dp->d_name, ".json" );
                if ( ( p == NULL ) || ( p[5] != 0 ) )
                {
                    continue;
                }

                if (pState->verbose )
                {
                    printf("VARCREATE: Creating vars: %s\n", pFilename );
                }

                /* create variables from file */
                rc = VARCREATE_CreateFromFile( hVarServer,
                                               pFilename,
                                               pOptions );
                if ( rc != EOK )
                {
                    fprintf( stderr,
                             "Failed to create variables from %s\n",
                             pFilename );
                    result = rc;
                }
            }
        }
        else
        {
            /* directory not found */
            result = ENOENT;
        }
    }

    return result;
}

/*============================================================================*/
/*  CreateFullPath                                                            */
/*!
    Create a full path from directory name and file name

    The CreateFullPath function concatenates the directory and file name
    together with a forward slash if appropriate.
    The full path is stored in static buffer so this function is not intended
    to be re-entrant, and the returned pointer to the full path should not
    be persisted.

    @param[in]
        dirname
            pointer to the directory name

    @param[in]
        filename
            pointer to the file name

    @retval pointer to the full path name
    @retval NULL if the total path length is exceeded, or the inputs are invalid

==============================================================================*/
static char *CreateFullPath( char *dirname, char *filename )
{
    char *fullpath = NULL;
    static char buf[BUFSIZ];
    size_t buflen = sizeof buf - 1;
    size_t len;
    int n;

    buf[buflen] = 0;

    if ( ( dirname != NULL ) && ( filename != NULL ) )
    {
        len = strlen( dirname );
        if ( dirname[len-1] == '/' )
        {
            n = snprintf( buf, buflen, "%s%s", dirname, filename );
        }
        else
        {
            n = snprintf( buf, buflen, "%s/%s", dirname, filename );
        }

        if ( ( n > 0 ) && ( (size_t)n <= buflen ) )
        {
            fullpath = buf;
        }
        else
        {
            fprintf( stderr, "File name too long: %s\n", filename );
        }
    }

    return fullpath;
}

/*! @}
 * end of varcreate group */

