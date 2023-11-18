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

#ifndef LIBVARCREATE_H
#define LIBVARCREATE_H

/*============================================================================
        Includes
============================================================================*/

#include <stdbool.h>
#include <stdint.h>
#include <varserver/varserver.h>
#include <varserver/var.h>

/*============================================================================
        Defines
============================================================================*/

#ifndef EOK
#define EOK 0
#endif

/*============================================================================
        Public Types
============================================================================*/

/*! The VarCreateOptions object is used to customize the
    variable creation */
typedef struct _VarCreateOptions
{
    /*! NUL terminated prefix to prepend to every variable name */
    char *prefix;

    /*! instance identifier to use for every variable in the file */
    uint32_t instanceID;

    /*! flags to apply to every variable in the file. */
    uint32_t flags;

    /*! enable verbose logging */
    bool verbose;

} VarCreateOptions;

/*============================================================================
        Public Function Declarations
============================================================================*/

int VARCREATE_CreateFromFile( VARSERVER_HANDLE hVarServer,
                              char *filename,
                              VarCreateOptions *options );

#endif
