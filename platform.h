/*
  This file contains platform dependend macros.
*/

#ifndef __platform_h__
#define __platform_h__

#if defined(linux) || defined(__linux) || defined(__linux__)
#undef __LINUX__
#define __LINUX__  1
#elif defined(__FreeBSD__) || defined(__DragonFly__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
#undef __BSD__
#define __BSD__    1
#endif


#endif // __platform_h__
