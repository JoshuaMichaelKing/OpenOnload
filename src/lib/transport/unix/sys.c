/*
** Copyright 2005-2017  Solarflare Communications Inc.
**                      7505 Irvine Center Drive, Irvine, CA 92618, USA
** Copyright 2002-2005  Level 5 Networks Inc.
**
** This program is free software; you can redistribute it and/or modify it
** under the terms of version 2 of the GNU General Public License as
** published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

/**************************************************************************\
*//*! \file
** <L5_PRIVATE L5_SOURCE>
** \author  
**  \brief  
**   \date  
**    \cop  (c) Level 5 Networks Limited.
** </L5_PRIVATE>
*//*
\**************************************************************************/
  
/*! \cidoxg_lib_transport_unix */
 
/* This is required for RTLD_NEXT from dlfcn.h */
#define _GNU_SOURCE

#include <internal.h>
#include <dlfcn.h>


static int load_sym_fail(const char* sym)
{
  Log_E(log("citp_find_calls: ERROR: dlsym(\"%s\") failed '%s'",
            sym, dlerror()));
  return -1;
}


static int
citp_find_all_sys_calls(void)
{
  /*
  ** RTLD_NEXT can be used in place of a library handle, and means 'find
  ** the next occurence of the symbol in the library search order'.
  ** However, it is only available in recent glibc (2.2.5 has it, 2.1.2
  ** doesn't).  If it is not defined, we have to open libc ourselves, and
  ** hope we get the name right!
  */

  /*
  ** NB. If RTLD_NEXT is not defined, we ought to look in libpthread before
  ** we go to libc, since the former adds thread cancellation tests to some
  ** of the sys calls (those prefixed by __libc_).
  */

#ifndef RTLD_NEXT
  void* dlhandle;
  const char* lib = "libc.so.6";  /* ?? */
  dlhandle = dlopen(lib, RTLD_NOW | RTLD_GLOBAL);
  if( dlhandle == 0 ) {    
    Log_E(log("%s: ERROR: dlopen(%s) failed dlerror=%s",
              __FUNCTION__, lib, dlerror()));
    return -1;
  }
# define CI_MK_DECL(ret, fn, args)              \
  ci_sys_##fn = dlsym(dlhandle, #fn);           \
  if( ci_sys_##fn == NULL )                     \
    return load_sym_fail(#fn);
#else
# define CI_MK_DECL(ret, fn, args)                                      \
  ci_sys_##fn = dlsym(RTLD_NEXT, #fn);                                  \
  if( ci_sys_##fn == NULL ) {                                           \
    /*                                                                  \
     * NOTE: the socket tester uses dlopen() on libcitransport.so and so \
     *       lookup using RTLD_NEXT may fail in this case. If it does then \
     *       try RTLD_DEFAULT to search all libraries.                  \
     */                                                                 \
    ci_sys_##fn = dlsym(RTLD_DEFAULT, #fn);                             \
  }                                                                     \
  if( ci_sys_##fn == NULL )                                             \
    return load_sym_fail(#fn);
#endif

#include <onload/declare_syscalls.h.tmpl>

#ifndef RTLD_NEXT
  if( dlclose(dlhandle) != 0 )
    Log_E(log("%s: ERROR: dlclose != 0", __FUNCTION__));
#endif

  return 0;
}


int
citp_syscall_init(void)
{
  if (citp_find_all_sys_calls() < 0)
    return -1;

  return 0;
}


#include <sys/stat.h>
# define ONLOAD_DEV       "/dev/onload"
# define citp_major(dev) ((dev) & 0xff00)

/*! \cidoxg_end */
