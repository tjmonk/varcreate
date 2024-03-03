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
 * @defgroup libvarcreate libvarcreate
 * @brief Shared object library to create variables from a JSON config file
 * @{
 */

/*============================================================================*/
/*!
@file libvarcreate.c

    Variable Creation from a JSON config file

    The Variable Creation library provides a mechanism to create
    varserver variables from a JSON configuration file.

    Each Variable is created via a call to the variable server API

*/
/*============================================================================*/


/*==============================================================================
        Includes
==============================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <varserver/var.h>
#include "cJSON.h"
#include <varcreate/libvarcreate.h>

/*==============================================================================
        Private definitions
==============================================================================*/

/*! specifies the maximum allowable file size of varcreate files */
#define MAX_VARCREATE_FILE_SIZE               ( 256 * 1024 )

/*==============================================================================
        Type Definitions
==============================================================================*/

/*! handler for the JSON variable attributes */
typedef struct _JSONHandler
{
    /*! name of the attribute to process */
    char *attribute;

    /*! function to handle the attribute */
    int (*fn)(VARSERVER_HANDLE, VarInfo *, cJSON *);

} JSONHandler;

/*==============================================================================
        Private function declarations
==============================================================================*/
static int varcreate_fnReadFile( char *filename,
                                 char** filedata,
                                 size_t *filesize );

static int varcreate_fnGetFileData( char *filename,
                                    char **filedata,
                                    size_t filesize );

static int varcreate_fnProcessVarData( VARSERVER_HANDLE hVarServer,
                                       cJSON *vardata,
                                       VarCreateOptions *options );

static int varcreate_fnProcessVar( VARSERVER_HANDLE hVarServer,
                                   const cJSON *vardata,
                                   VarCreateOptions *options );

static int varcreate_ProcessName( VARSERVER_HANDLE hVarServer,
                                  VarInfo *pVarInfo,
                                  cJSON *name );

static int varcreate_ProcessGUID( VARSERVER_HANDLE hVarServer,
                                  VarInfo *pVarInfo,
                                  cJSON *guid );

static int varcreate_ProcessFormat( VARSERVER_HANDLE hVarServer,
                                    VarInfo *pVarInfo,
                                    cJSON *format );

static int varcreate_ProcessLength( VARSERVER_HANDLE hVarServer,
                                    VarInfo *pVarInfo,
                                    cJSON *length );

static int varcreate_ProcessValue( VARSERVER_HANDLE hVarServer,
                                   VarInfo *pVarInfo,
                                   cJSON *value );

static int varcreate_ProcessType( VARSERVER_HANDLE hVarServer,
                                  VarInfo *pVarInfo,
                                  cJSON *type );

static int varcreate_ProcessTags( VARSERVER_HANDLE hVarServer,
                                  VarInfo *pVarInfo,
                                  cJSON *name );

static int varcreate_ProcessReadPermissions( VARSERVER_HANDLE hVarServer,
                                             VarInfo *pVarInfo,
                                             cJSON *read );

static int varcreate_ProcessWritePermissions( VARSERVER_HANDLE hVarServer,
                                              VarInfo *pVarInfo,
                                              cJSON *write );

static int varcreate_ProcessFlags( VARSERVER_HANDLE hVarServer,
                                   VarInfo *pVarInfo,
                                   cJSON *flags );

static int varcreate_ProcessAlias( VARSERVER_HANDLE hVarServer,
                                   VarInfo *pVarInfo,
                                   cJSON *alias );

static int varcreate_ProcessDescription( VARSERVER_HANDLE hVarServer,
                                         VarInfo *pVarInfo,
                                         cJSON *description );

static int varcreate_ProcessShortName( VARSERVER_HANDLE hVarServer,
                                       VarInfo *pVarInfo,
                                       cJSON *shortName );

/*==============================================================================
        Function definitions
==============================================================================*/

void __attribute__ ((constructor)) initLibrary(void) {
 //
 // Function that is called when the library is loaded
 //
}
void __attribute__ ((destructor)) cleanUpLibrary(void) {
 //
 // Function that is called when the library is »closed«.
 //
}

/*============================================================================*/
/*  VARCREATE_CreateFromFile                                                  */
/*!
    Create variables from a JSON config file

    The VARCREATE_CreateFromFile function dynamically creates varserver
    variables at run-time by parsing a JSON configuration file.

    @param[in]
        hVarServer
            handle to the Variable Server to create variables for

    @param[in]
        filename
            name of the variable creation JSON config file

    @param[in]
        options
            pointer to the options used to modify the variable creation
            behavior.

    @retval EOK - variable creation was successful
    @retval EINVAL - invalid arguments
    @retval ENOMEM - memory allocation problem

==============================================================================*/
int VARCREATE_CreateFromFile( VARSERVER_HANDLE hVarServer,
                              char *filename,
                              VarCreateOptions *options )
{
    int result = EINVAL;
    char *filedata;
    size_t filesize;

    /* read the varcreate file */
    result = varcreate_fnReadFile( filename, &filedata, &filesize );
    if( result == EOK )
    {
        /* parse the JSON data read from the file */
        result = VARCREATE_CreateFromString(hVarServer, filedata, options);
    }

    return result;
}

/*==========================================================================*/
/*  VARCREATE_CreateFromString                                              */
/*!
    Create variables from a string containing JSON configuration

    The VARCREATE_CreateFromString function dynamically creates varserver
    variables at run-time by parsing a JSON configuration string.

    @param[in]
        hVarServer
            handle to the Variable Server to create variables for

    @param[in]
        filedata
            string containing variable creation JSON config data

    @param[in]
        options
            pointer to the options used to modify the variable creation
            behavior.

    @retval EOK - variable creation was successful
    @retval EINVAL - invalid arguments

============================================================================*/
int VARCREATE_CreateFromString( VARSERVER_HANDLE hVarServer,
                                const char *filedata,
                                VarCreateOptions *options )
{
    int result = EINVAL;
    cJSON *vardata;
    const char *error_ptr;

    vardata = cJSON_Parse( filedata );
    if( vardata != NULL )
    {
        /* process the variable data */
        varcreate_fnProcessVarData( hVarServer, vardata, options );

        /* delete the vardata JSON object now that we are done with it */
        cJSON_Delete( vardata );

        /* indicate success */
        result = EOK;
    }
    else
    {
        /* parsing failed, find out where */
        error_ptr = cJSON_GetErrorPtr();
        if( error_ptr != NULL )
        {
            /* indicate the error to the user */
            fprintf(stderr, "Error before: %s\n", error_ptr );
            result = EBADMSG;
        }
    }

    return result;
}

/*============================================================================*/
/*  varcreate_fnProcessVarData                                                */
/*!
    Process the variables specified in the varcreate JSON

    The varcreate_fnProcessVarData function processes the varcreates JSON
    object specified in vardata, iterates through the variables in the
    JSON array and creates them via a call to the variable server.

    @param[in]
        hVarServer
            handle to the Variable Server to create variables for

    @param[in]
        vardata
            pointer to a cJSON object containing the variable data

    @param[in]
        options
            pointer to the options used to modify the variable creation
            behavior.

    @retval EOK - variable creation was successful
    @retval EINVAL - invalid arguments
    @retval ENOMEM - memory allocation problem

==============================================================================*/
static int varcreate_fnProcessVarData( VARSERVER_HANDLE hVarServer,
                                       cJSON *vardata,
                                       VarCreateOptions *options )
{
    int result = EINVAL;
    const cJSON *desc;
    const cJSON *vars;
    const cJSON *var;
    int rc;

    if( ( vardata != NULL ) &&
        ( options != NULL ) )
    {
        /* get the description of the variables being created */
        desc = cJSON_GetObjectItemCaseSensitive( vardata,
                                                 "description" );

        /* not doing anything with desc right now */
        (void)desc;

        /* get a pointer to the array of variables to be created */
        vars = cJSON_GetObjectItemCaseSensitive( vardata, "vars" );
        if( ( vars != NULL ) &&
            ( cJSON_IsArray( vars ) ) )
        {
            result = EOK;

            /* iterate through each variable to be created */
            cJSON_ArrayForEach( var, vars )
            {
                /* process each variable one at a time */
                rc = varcreate_fnProcessVar( hVarServer, var, options );
                if( rc != EOK )
                {
                    /* variable creation failed */
                    result = rc;
                }
            }
        }
    }

    return result;
}

/*============================================================================*/
/*  varcreate_fnProcessVar                                                    */
/*!
    Process a variable specified in the varcreate JSON

    The varcreate_fnProcessVar function processes a single variable
    in the specified JSON object.
}    @param[in]
        hVarServer
            handle to the Variable Server to create variables for

    @param[in]
        vardata
            pointer to a cJSON object containing the variable data

    @param[in]
        options
            pointer to the options used to modify the variable creation
            behavior.

    @retval EOK - variable creation was successful
    @retval EINVAL - invalid arguments
    @retval ENOMEM - memory allocation problem

==============================================================================*/
static int varcreate_fnProcessVar( VARSERVER_HANDLE hVarServer,
                                   const cJSON *vardata,
                                   VarCreateOptions *options )
{
    cJSON *item;
    int i = 0;
    int rc;
    VarInfo variableInfo;
    int result = EINVAL;
    char buf[MAX_NAME_LEN+1];
    size_t len;

    JSONHandler handlers[] =
        {
            { "name", varcreate_ProcessName },
            { "guid", varcreate_ProcessGUID },
            { "type", varcreate_ProcessType },
            { "fmt", varcreate_ProcessFormat },
            { "length", varcreate_ProcessLength },
            { "value", varcreate_ProcessValue },
            { "tags", varcreate_ProcessTags },
            { "flags", varcreate_ProcessFlags },
            { "description", varcreate_ProcessDescription },
            { "shortname", varcreate_ProcessShortName },
            { "read", varcreate_ProcessReadPermissions },
            { "write", varcreate_ProcessWritePermissions },
            { NULL, NULL }
        };

    if( ( hVarServer != NULL ) &&
        ( vardata != NULL ) &&
        ( cJSON_IsObject( vardata ) ) &&
        ( options != NULL ) )
    {
        /* clear the Variable Info object */
        memset( &variableInfo, 0, sizeof( VarInfo ) );

        /* assume success until something fails */
        result = EOK;

        /* process the attributes */
        while( handlers[i].attribute != NULL )
        {
            /* get the JSON object item */
            item = cJSON_GetObjectItem( vardata, handlers[i].attribute );
            if( item != NULL )
            {
                /* process the object item */
                rc = handlers[i].fn( hVarServer, &variableInfo, item );
                if( rc != EOK )
                {
                    printf("Failed handler: %s\n", handlers[i].attribute );
                    result = rc;
                }
            }

            /* move to the next JSON attribute */
            i++;
        }

        if ( options->flags )
        {
            variableInfo.flags |= (options->flags);
        }

        if ( ( variableInfo.var.type == VARTYPE_STR ) &&
             ( variableInfo.var.len > 0 ) )
        {
            /* increment the length by 1 to account for the NUL terminator */
            variableInfo.var.len++;

            if ( variableInfo.var.val.str != NULL )
            {
                len = strlen( variableInfo.var.val.str );
                if ( len >= variableInfo.var.len )
                {
                    printf( "Value too large for variable: %s\n",
                            variableInfo.name );
                }
            }
        }

        variableInfo.instanceID = options->instanceID;

        if ( options->prefix != NULL )
        {
            /* prepend the variable name with the variable prefix */
            snprintf( buf,
                      MAX_NAME_LEN+1,
                      "%s%s",
                      options->prefix,
                      variableInfo.name );
            strcpy( variableInfo.name, buf );
        }

        if( result == EOK )
        {
            /* create the variable */
            if ( options->verbose )
            {
                printf("VARCREATE: Creating variable: %s\n", variableInfo.name);
            }

            result = VARSERVER_CreateVar( hVarServer, &variableInfo );
            if ( ( result == EOK ) && ( variableInfo.hVar != VAR_INVALID ) )
            {
                /* check for aliases */
                item = cJSON_GetObjectItem( vardata, "alias" );
                if ( item != NULL )
                {
                    result = varcreate_ProcessAlias( hVarServer,
                                                     &variableInfo,
                                                     item );
                    if ( result != EOK )
                    {
                        printf("Failed handler: alias\n" );
                    }
                }
            }
            else
            {
                printf("Failed to create variable: %s\n", variableInfo.name );
            }
        }
    }

    return result;
}

/*============================================================================*/
/*  varcreate_ProcessName                                                     */
/*!
    Process a variable name from the varcreate JSON object

    The varcreate_fnProcessName function processes a the variable
    name from the varcreate JSON object

    @param[in]
        hVarServer
            handle to the Variable Server to create variables for

    @param[in]
        pVarInfo
            pointer to the VarInfo object where the name will be stored

    @param[in]
        name
            pointer to the cJSON object attribute 'name' to be processed

    @retval EOK - variable name was copied successfully
    @retval E2BIG - the variable name was too long
    @retval EINVAL - invalid arguments

==============================================================================*/
static int varcreate_ProcessName( VARSERVER_HANDLE hVarServer,
                                  VarInfo *pVarInfo,
                                  cJSON *name )
{
    int result = EINVAL;
    size_t len;

    /* hVarServer unused */
    (void)hVarServer;

    if( ( pVarInfo != NULL ) &&
        ( name != NULL ) )
    {
        if( ( cJSON_IsString( name ) ) &&
            ( name->valuestring != NULL ) )
        {
            /* get the variable name length */
            len = strlen( name->valuestring );
            if( len <= MAX_NAME_LEN )
            {
                /* store the variable name into the VarInfo object */
                memcpy( pVarInfo->name, name->valuestring, len );
                pVarInfo->name[len] = 0;
                result = EOK;
            }
            else
            {
                /* variable name exceeds the maximum size */
                result = E2BIG;
            }
        }
    }

    return result;
}

/*============================================================================*/
/*  varcreate_ProcessDescription                                              */
/*!
    Process a variable description from the varcreate JSON object

    The varcreate_fnProcessDescription function processes the variable
    description from the varcreate JSON object

    @param[in]
        hVarServer
            handle to the Variable Server to create variables for

    @param[in]
        pVarInfo
            pointer to the VarInfo object

    @param[in]
        description
            pointer to the cJSON object attribute 'description'
            to be processed

    @retval EOK - variable description was copied successfully
    @retval E2BIG - the variable description was too long
    @retval EINVAL - invalid arguments

==============================================================================*/
static int varcreate_ProcessDescription( VARSERVER_HANDLE hVarServer,
                                         VarInfo *pVarInfo,
                                         cJSON *description )
{
    int result = EINVAL;

    /* hVarServer unused */
    (void)hVarServer;

    if( ( pVarInfo != NULL ) &&
        ( description != NULL ) )
    {
        if( ( cJSON_IsString( description ) ) &&
            ( description->valuestring != NULL ) )
        {
            result = EOK;
        }
    }

    return result;
}

/*============================================================================*/
/*  varcreate_ProcessType                                                     */
/*!
    Process a variable type from the varcreate JSON object

    The varcreate_fnProcessType function processes the variable
    type from the varcreate JSON object

    @param[in]
        hVarServer
            handle to the Variable Server to create variables for

    @param[in]
        pVarInfo
            pointer to the VarInfo object

    @param[in]
        type
            pointer to the cJSON object attribute 'type'
            to be processed

    @retval EOK - variable type was processed successfully
    @retval ENOTSUP - the variable type is not supported
    @retval EINVAL - invalid arguments

==============================================================================*/
static int varcreate_ProcessType( VARSERVER_HANDLE hVarServer,
                                  VarInfo *pVarInfo,
                                  cJSON *type )
{
    int result = EINVAL;
    char buf[64];

    /* hVarServer unused */
    (void)hVarServer;

    if( ( pVarInfo != NULL ) &&
        ( type != NULL ) )
    {
        if( ( cJSON_IsString( type ) ) &&
            ( type->valuestring != NULL ) )
        {
            result = VARSERVER_TypeNameToType( type->valuestring,
                                               &pVarInfo->var.type );

            VARSERVER_TypeToTypeName( pVarInfo->var.type,
                                      buf,
                                      sizeof(buf) );

            result = EOK;
        }
    }

    return result;
}

/*============================================================================*/
/*  varcreate_ProcessFormat                                                   */
/*!
    Process a variable format specifier from the varcreate JSON object

    The varcreate_fnProcessFormat function processes the variable
    format specifier from the varcreate JSON object

    @param[in]
        hVarServer
            handle to the Variable Server to create variables for

    @param[in]
        pVarInfo
            pointer to the VarInfo object

    @param[in]
        fmt
            pointer to the cJSON object attribute 'fmt'
            to be processed

    @retval EOK - variable format specifier was processed successfully
    @retval E2BIG - the format specifier exceeds the maximum size
    @retval EINVAL - invalid arguments

==============================================================================*/
static int varcreate_ProcessFormat( VARSERVER_HANDLE hVarServer,
                                    VarInfo *pVarInfo,
                                    cJSON *fmt )
{
    int result = EINVAL;
    size_t len;

    /* hVarServer unused */
    (void)hVarServer;

    if( ( pVarInfo != NULL ) &&
        ( fmt != NULL ) )
    {
        if( ( cJSON_IsString( fmt ) ) &&
            ( fmt->valuestring != NULL ) )
        {
            len = strlen( fmt->valuestring );
            if( len < MAX_FORMATSPEC_LEN )
            {
                /* copy the format specifier */
                strcpy( pVarInfo->formatspec, fmt->valuestring );
                result = EOK;
            }
            else
            {
                result = E2BIG;
            }
        }
    }

    return result;
}

/*============================================================================*/
/*  varcreate_ProcessLength                                                   */
/*!
    Process a variable length specifier from the varcreate JSON object

    The varcreate_fnProcessLength function processes the variable
    length specifier from the varcreate JSON object

    @param[in]
        hVarServer
            handle to the Variable Server to create variables for

    @param[in]
        pVarInfo
            pointer to the VarInfo object

    @param[in]
        length
            pointer to the cJSON object attribute 'length'
            to be processed

    @retval EOK - variable length was processed successfully
    @retval E2BIG - the length specifier exceeds the maximum size
    @retval EINVAL - invalid arguments

==============================================================================*/
static int varcreate_ProcessLength( VARSERVER_HANDLE hVarServer,
                                    VarInfo *pVarInfo,
                                    cJSON *length )
{
    int result = EINVAL;
    int base = 0;

    /* hVarServer unused */
    (void)hVarServer;

    if( ( pVarInfo != NULL ) &&
        ( length != NULL ) )
    {
        if( ( cJSON_IsString( length ) ) &&
            ( length->valuestring != NULL ) )
        {
            if( ( length->valuestring[0] == '0' ) &&
                ( tolower(length->valuestring[1]) == 'x' ) )
            {
                base = 16;
            }

            pVarInfo->var.len = strtoul( length->valuestring, NULL, base );
            result = EOK;
        }
    }

    return result;
}

/*============================================================================*/
/*  varcreate_ProcessValue                                                    */
/*!
    Process a variable value from the varcreate JSON object

    The varcreate_fnProcessValue function processes the variable
    initial value from the varcreate JSON object

    @param[in]
        hVarServer
            handle to the Variable Server to create variables for

    @param[in]
        pVarInfo
            pointer to the VarInfo object

    @param[in]
        value
            pointer to the cJSON object attribute 'value'
            to be processed

    @retval EOK - variable value was processed successfully
    @retval ENOTSUP - the variable value is not supported
    @retval EINVAL - invalid arguments

==============================================================================*/
static int varcreate_ProcessValue( VARSERVER_HANDLE hVarServer,
                                   VarInfo *pVarInfo,
                                   cJSON *value )
{
    int result = EINVAL;

    (void)hVarServer;

    if( ( pVarInfo != NULL ) &&
        ( value != NULL ) )
    {
        if( ( cJSON_IsString( value ) ) &&
            ( value->valuestring != NULL ) )
        {
            if( pVarInfo->var.type == VARTYPE_STR )
            {
                /* store the value to set */
                pVarInfo->var.val.str = value->valuestring;

                result = EOK;
            }
            else
            {
                result = VAROBJECT_CreateFromString( value->valuestring,
                                                     pVarInfo->var.type,
                                                     &pVarInfo->var,
                                                     0 );
            }
        }
    }

    return result;
}

/*============================================================================*/
/*  varcreate_ProcessShortName                                                */
/*!
    Process a variable short name from the varcreate JSON object

    The varcreate_fnProcessShortName function processes the variable
    shortname attribute from the varcreate JSON object

    @param[in]
        hVarServer
            handle to the Variable Server to create variables for

    @param[in]
        pVarInfo
            pointer to the VarInfo object

    @param[in]
        shortName
            pointer to the cJSON object attribute 'shortname'
            to be processed

    @retval EOK - variable short name was copied successfully
    @retval E2BIG - the variable short name was too long
    @retval EINVAL - invalid arguments

==============================================================================*/
static int varcreate_ProcessShortName( VARSERVER_HANDLE hVarServer,
                                       VarInfo *pVarInfo,
                                       cJSON *shortName )
{
    int result = EINVAL;

    /* hVarServer unused */
    (void)hVarServer;

    if( ( pVarInfo != NULL ) &&
        ( shortName != NULL ) )
    {
        if( ( cJSON_IsString( shortName ) ) &&
            ( shortName->valuestring != NULL ) )
        {
            result = EOK;
        }
    }

    return result;
}

/*============================================================================*/
/*  varcreate_ProcessGUID                                                     */
/*!
    Process a variable GUID from the varcreate JSON object

    The varcreate_fnProcessGUID function processes the variable
    GUID 32-bit hexadecimal attribute from the varcreate JSON object

    @param[in]
        hVarServer
            handle to the Variable Server to create variables for

    @param[in]
        pVarInfo
            pointer to the VarInfo object

    @param[in]
        guid
            pointer to the cJSON object attribute 'guid'
            to be processed

    @retval EOK - variable GUID was processed successfully
    @retval EINVAL - invalid arguments

==============================================================================*/
static int varcreate_ProcessGUID( VARSERVER_HANDLE hVarServer,
                                  VarInfo *pVarInfo,
                                  cJSON *guid )
{
    int result = EINVAL;

    /* hVarServer unused */
    (void)hVarServer;

    if( ( pVarInfo != NULL ) &&
        ( guid != NULL ) )
    {
        if( ( cJSON_IsString( guid ) ) &&
            ( guid->valuestring != NULL ) )
        {
            pVarInfo->guid = strtoul( guid->valuestring, NULL, 16 );
            result = EOK;
        }
    }

    return result;
}

/*============================================================================*/
/*  varcreate_ProcessTags                                                     */
/*!
    Process variable tags from the varcreate JSON object

    The varcreate_fnProcessTags function processes the variable
    tags from the varcreate JSON object

    @param[in]
        hVarServer
            handle to the Variable Server to create variables for

    @param[in]
        pVarInfo
            pointer to the VarInfo object where the tags will be stored

    @param[in]
        name
            pointer to the cJSON object attribute 'tags' to be processed

    @retval EOK - variable tags were copied successfully
    @retval E2BIG - the variable tags were too long
    @retval EINVAL - invalid arguments

==============================================================================*/
static int varcreate_ProcessTags( VARSERVER_HANDLE hVarServer,
                                  VarInfo *pVarInfo,
                                  cJSON *tagspec )
{
    int result = EINVAL;
    size_t len;

    /* hVarServer unused */
    (void)hVarServer;

    if( ( pVarInfo != NULL ) &&
        ( tagspec != NULL ) )
    {
        if( ( cJSON_IsString( tagspec ) ) &&
            ( tagspec->valuestring != NULL ) )
        {
            /* get the variable name length */
            len = strlen( tagspec->valuestring );
            if( len < MAX_TAGSPEC_LEN )
            {
                /* store the variable name into the VarInfo object */
                memcpy( pVarInfo->tagspec, tagspec->valuestring, len );
                pVarInfo->tagspec[len] = 0;
                result = EOK;
            }
            else
            {
                /* variable name exceeds the maximum size */
                result = E2BIG;
            }
        }
    }

    return result;
}

/*============================================================================*/
/*  varcreate_ProcessFlags                                                    */
/*!
    Process variable flags from the varcreate JSON object

    The varcreate_fnProcessFlags function processes the variable
    flags from the varcreate JSON object

    @param[in]
        hVarServer
            handle to the Variable Server to create variables for

    @param[in]
        pVarInfo
            pointer to the VarInfo object where the flags will be stored

    @param[in]
        name
            pointer to the cJSON object attribute 'flags' to be processed

    @retval EOK - variable flags were parsed successfully
    @retval E2BIG - the variable flags were too long
    @retval EINVAL - invalid arguments
    @retval ENOENT - one or more flag names were invalid

==============================================================================*/
static int varcreate_ProcessFlags( VARSERVER_HANDLE hVarServer,
                                   VarInfo *pVarInfo,
                                   cJSON *flags )
{
    int result = EINVAL;

    /* hVarServer unused */
    (void)hVarServer;

    if( ( pVarInfo != NULL ) &&
        ( flags != NULL ) )
    {
        if( ( cJSON_IsString( flags ) ) &&
            ( flags->valuestring != NULL ) )
        {
            result = VARSERVER_StrToFlags( flags->valuestring,
                                           &pVarInfo->flags );
            if ( result != EOK )
            {
                fprintf(stderr,
                        "Failed to convert flag string: %s, "
                        "flag value: %" PRIu32 ", "
                        "result: %d %s\n",
                        flags->valuestring,
                        pVarInfo->flags,
                        result,
                        strerror( result ) );
            }
        }
    }

    return result;
}

/*============================================================================*/
/*  varcreate_ProcessAlias                                                    */
/*!
    Process variable aliases from the varcreate JSON object

    The varcreate_fnProcessAlias function processes the variable
    aliases from the varcreate JSON object

    @param[in]
        hVarServer
            handle to the Variable Server to create variables for

    @param[in]
        pVarInfo
            pointer to the VarInfo object where the flags will be stored

    @param[in]
        aliases
            pointer to the cJSON object attribute 'alias' to be processed

    @retval EOK - variable flags were parsed successfully
    @retval E2BIG - the variable flags were too long
    @retval EINVAL - invalid arguments
    @retval ENOENT - one or more flag names were invalid

==============================================================================*/
static int varcreate_ProcessAlias( VARSERVER_HANDLE hVarServer,
                                   VarInfo *pVarInfo,
                                   cJSON *alias )
{
    int result = EINVAL;
    int n;
    int i;
    cJSON *item;
    VAR_HANDLE hVar;
    int rc;

    /* hVarServer unused */
    (void)hVarServer;

    if( ( pVarInfo != NULL ) &&
        ( alias != NULL ) )
    {
        hVar = pVarInfo->hVar;

        if( ( cJSON_IsString( alias ) ) &&
            ( alias->valuestring != NULL ) )
        {
            result = VAR_Alias( hVarServer, hVar, alias->valuestring, NULL );
        }
        else if ( cJSON_IsArray( alias ) )
        {
            n = cJSON_GetArraySize( alias );
            for ( i = 0 ; i < n ; i ++ )
            {
                result = EOK;

                item = cJSON_GetArrayItem( alias, i );
                if ( ( item != NULL ) &&
                     ( item->valuestring != NULL ) )
                {
                    rc = VAR_Alias( hVarServer, hVar, item->valuestring, NULL );
                    if ( rc != EOK )
                    {
                        result = rc;
                    }
                }
            }
        }
    }

    return result;
}

/*============================================================================*/
/*  varcreate_ProcessReadPermissions                                          */
/*!
    Process a variable read permissions from the varcreate JSON object

    The varcreate_ProcessReadPermissions function processes the variable
    read permissions list (comma separated list of user IDs) from the
    varcreate JSON object

    @param[in]
        hVarServer
            handle to the Variable Server to create variables for

    @param[in]
        pVarInfo
            pointer to the VarInfo object

    @param[in]
        read
            pointer to the cJSON object attribute 'read'
            to be processed

    @retval EOK - variable value was processed successfully
    @retval E2BIG - permissions list is too long
    @retval EINVAL - invalid arguments

==============================================================================*/
static int varcreate_ProcessReadPermissions( VARSERVER_HANDLE hVarServer,
                                             VarInfo *pVarInfo,
                                             cJSON *read )
{
    int result = EINVAL;

    /* hVarServer unused */
    (void)hVarServer;

    if( ( pVarInfo != NULL ) &&
        ( read != NULL ) )
    {
        if( ( cJSON_IsString( read ) ) &&
            ( read->valuestring != NULL ) )
        {
            pVarInfo->permissions.nreads = VARSERVER_MAX_UIDS;
            result = VARSERVER_ParsePermissionSpec(
                                    read->valuestring,
                                    pVarInfo->permissions.read,
                                    &pVarInfo->permissions.nreads );
        }
    }

    return result;
}

/*============================================================================*/
/*  varcreate_ProcessWritePermissions                                         */
/*!
    Process a variable write permissions from the varcreate JSON object

    The varcreate_ProcessWritePermissions function processes the variable
    write permissions list (comma separated list of user IDs) from the
    varcreate JSON object

    @param[in]
        hVarServer
            handle to the Variable Server to create variables for

    @param[in]
        pVarInfo
            pointer to the VarInfo object

    @param[in]
        write
            pointer to the cJSON object attribute 'write'
            to be processed

    @retval EOK - variable value was processed successfully
    @retval E2BIG - permissions list is too long
    @retval EINVAL - invalid arguments

==============================================================================*/
static int varcreate_ProcessWritePermissions( VARSERVER_HANDLE hVarServer,
                                              VarInfo *pVarInfo,
                                              cJSON *write )
{
    int result = EINVAL;

    /* hVarServer unused */
    (void)hVarServer;

    if( ( pVarInfo != NULL ) &&
        ( write != NULL ) )
    {
        if( ( cJSON_IsString( write ) ) &&
            ( write->valuestring != NULL ) )
        {
            pVarInfo->permissions.nwrites = VARSERVER_MAX_UIDS;
            result = VARSERVER_ParsePermissionSpec(
                                    write->valuestring,
                                    pVarInfo->permissions.write,
                                    &pVarInfo->permissions.nwrites );
        }
    }

    return result;
}

/*============================================================================*/
/*  varcreate_fnReadFile                                                      */
/*!
    read a file into a buffer

    The varcreate_fnReadFile is a helper function to read a file
   specified by its filename into a buffer, provided it is smaller than
    the maximum allowable file size defined by MAX_VARCREATE_FILE_SIZE

    @param[in]
        filename
            pointer to the name of the file to read

    @param[in]
        filedata
            pointer to a location to store the file data pointer

    @param[in]
        filesize
            pointer to a location to store the file size

    @retval EOK - the file content was read into memory
    @retval EINVAL - invalid arguments
    @retval ENOMEM - memory allocation problem
    @retval ENOTSUP - cannot read this file type
    @retval ENOENT - unable to stat or open the file

==============================================================================*/
static int varcreate_fnReadFile( char *filename,
                                 char** filedata,
                                 size_t *filesize )
{
    int result = EINVAL;
    struct stat st;

    if( ( filename != NULL ) &&
        ( filedata != NULL ) &&
        ( filesize != NULL ) )
    {
        /* get information about the file including its size */
        result = stat( filename, &st );
        if( result != -1 )
        {
            /* check if this is a regular file */
            if (S_ISREG(st.st_mode))
            {
                /* get the file data */
                result = varcreate_fnGetFileData( filename,
                                                  filedata,
                                                  st.st_size );
                if( result == EOK )
                {
                    *filesize = st.st_size;
                    result = EOK;
                }
            }
            else
            {
                /* invalid file type */
                result = ENOTSUP;
            }
        }
        else
        {
            /* cannot stat file */
            result = ENOENT;
        }
    }

    return result;
}

/*============================================================================*/
/*  varcreate_fnGetFileData                                                   */
/*!
    read a file into a buffer

    The varcreate_fnGetFileData is a helper function to read a file
    specified by its filename into a buffer, provided it is smaller than
    the maximum allowable file size defined by MAX_VARCREATE_FILE_SIZE

    @param[in]
        filename
            pointer to the name of the file to read

    @param[in]
        filedata
            pointer to a location to store the file data pointer

    @param[in]
        filesize
            number of bytes of the file to read

    @retval EOK - the file content was read into memory
    @retval EINVAL - invalid arguments
    @retval ENOMEM - memory allocation problem
    @retval ENOTSUP - invalid file size
    @retval ENOENT - unable to open the file for reading
    @retval EIO - read operation failed

==============================================================================*/
static int varcreate_fnGetFileData( char *filename,
                                    char **filedata,
                                    size_t filesize )
{
    int result = EINVAL;
    FILE *fp;
    size_t bytesread;

    if( ( filename != NULL ) &&
        ( filedata != NULL ) )
    {
        if( filesize <= MAX_VARCREATE_FILE_SIZE )
        {
            /* Handle regular file */
            fp = fopen( filename, "r" );
            if( fp != NULL )
            {
                /* allocate memory to store the file */
                *filedata = malloc( filesize );
                if( *filedata != NULL )
                {
                    bytesread = fread( *filedata,
                                        1,
                                        filesize,
                                        fp );
                    if( bytesread != filesize )
                    {
                        /* free allocated memory */
                        free( *filedata );
                        *filedata = NULL;
                        result = EIO;
                    }
                    else
                    {
                        result = EOK;
                    }
                }
                else
                {
                    result = ENOMEM;
                }
            }
            else
            {
                /* file cannot be opened for reading */
                result = ENOENT;
            }
        }
        else
        {
            /* invalid file size */
            result = ENOTSUP;
        }
    }

    return result;
}

/*! @}
 * end of libvarcreate group */
