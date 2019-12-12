#ifndef _OS_H
#define _OS_H
/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis 'TREMOR' CODEC SOURCE CODE.   *
 *                                                                  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis 'TREMOR' SOURCE CODE IS (C) COPYRIGHT 1994-2002    *
 * BY THE Xiph.Org FOUNDATION http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: #ifdef jail to whip a few platforms into the UNIX ideal.

 ********************************************************************/

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <math.h>
#include <ogg/os_types.h>

#ifndef _V_IFDEFJAIL_H_
#  define _V_IFDEFJAIL_H_

#  ifdef __GNUC__
#    define STIN static __inline__
#  elif defined(_WIN32)
#    define STIN static __inline
#  else
#    define STIN static
#  endif
#endif

#ifdef _WIN32
#  include <malloc.h>
#endif

#if defined HAVE_ALLOCA

# ifdef _WIN32
#  include <malloc.h>
#  define VAR_STACK(type, var, size) type *var = ((type*)_alloca(sizeof(type)*(size)))
# else
#  ifdef HAVE_ALLOCA_H
#   include <alloca.h>
#  else
#   include <stdlib.h>
#  endif
#  define VAR_STACK(type, var, size) type *var = ((type*) alloca(sizeof(type)*(size)))
# endif

#elif defined VAR_ARRAYS

#  define VAR_STACK(type, var, size) type var[size]

#else

#error "Either VAR_ARRAYS or HAVE_ALLOCA must be defined to select the stack allocation mode"
#endif

#ifdef USE_MEMORY_H
#  include <memory.h>
#endif

#ifndef min
#  define min(x,y)  ((x)>(y)?(y):(x))
#endif

#ifndef max
#  define max(x,y)  ((x)<(y)?(y):(x))
#endif

#endif /* _OS_H */
