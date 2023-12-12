/****************************************************************************
*
*    Copyright 2012 - 2022 Vivante Corporation, Santa Clara, California.
*    All Rights Reserved.
*
*    Permission is hereby granted, free of charge, to any person obtaining
*    a copy of this software and associated documentation files (the
*    'Software'), to deal in the Software without restriction, including
*    without limitation the rights to use, copy, modify, merge, publish,
*    distribute, sub license, and/or sell copies of the Software, and to
*    permit persons to whom the Software is furnished to do so, subject
*    to the following conditions:
*
*    The above copyright notice and this permission notice (including the
*    next paragraph) shall be included in all copies or substantial
*    portions of the Software.
*
*    THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
*    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
*    IN NO EVENT SHALL VIVANTE AND/OR ITS SUPPLIERS BE LIABLE FOR ANY
*    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
*    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
*    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*****************************************************************************/

#ifndef _vg_lite_options_h_
#define _vg_lite_options_h_

// #define TOSTR(s)     #s

// #define _VGLITE_OPTIONS   _vg_lite_options.h
// #define _STRCAT(a,b)  a##b
// #define STRCAT(a,b)  _STRCAT(a,b)
// #define VG_OPTIONS_FILE(s) TOSTR(s)


#if defined (CHIP_BEST1502X)
#include <best1502x_vg_lite_options.h>
#elif defined (CHIP_BEST1600)
#include <best1600_vg_lite_options.h>
#elif defined (CHIP_BEST1700)
#include <best1700_vg_lite_options.h>
#else
#include <default_vg_lite_options.h>
#endif


#endif //_vg_lite_options_h_
