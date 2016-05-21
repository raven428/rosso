/*
  This functions are used to convert endianness of integers.
*/

#include "endianness.h"
#include "mallocv.h"

// Endian swap functions
#ifdef __BIG_ENDIAN__

u_int16_t SwapInt16(u_int16_t value) {
/*
  swaps endianness of a 16 bit integer
*/
  union {
    u_int16_t ivalue;
    char cvalue[2];
  } u;

  u.ivalue=value;

  char tmp;
  tmp=u.cvalue[0];
  u.cvalue[0]=u.cvalue[1];
  u.cvalue[1]=tmp;

  return u.ivalue;
}

u_int32_t SwapInt32(u_int32_t value) {
/*
  swaps endianness of a 32 bit integer
*/
  union {
    u_int32_t ivalue;
    char cvalue[4];
  } u;

  u.ivalue=value;

  char tmp;
  tmp=u.cvalue[0];
  u.cvalue[0]=u.cvalue[3];
  u.cvalue[3]=tmp;
  tmp=u.cvalue[1];
  u.cvalue[1]=u.cvalue[2];
  u.cvalue[2]=tmp;

  return u.ivalue;
}
#endif

