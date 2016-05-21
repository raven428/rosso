/*
  This functions are used to convert endianness of integers.
*/

#ifndef __endianness_h__
#define __endianness_h__

/*
  supports different byte-orders
*/

#ifndef __BIG_ENDIAN__
#define SwapInt16(i) i
#define SwapInt32(i) i
#else

#include <sys/types.h>
#include "platform.h"

// swaps endianness of a 16 bit integer
u_int16_t SwapInt16(u_int16_t value);

// swaps endianness of a 32 bit integer
u_int32_t SwapInt32(u_int32_t value);

#endif

#endif //__ endianness_h__
