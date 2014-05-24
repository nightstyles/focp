
#include "AFC.hpp"

#ifndef _EVM_DEF_HPP_
#define _EVM_DEF_HPP_

#ifdef EVM_EXPORTS
#define EVM_API FOCP_EXPORT
#else
#define EVM_API FOCP_IMPORT
#endif

FOCP_BEGIN();

typedef int8 ehc_char;
typedef uint8 ehc_uchar;
typedef int16 ehc_short;
typedef uint16 ehc_ushort;
typedef int32 ehc_int;
typedef uint32 ehc_uint;
typedef int64 ehc_long;
typedef uint64 ehc_ulong;
typedef float ehc_float;
typedef double ehc_double;
typedef void ehc_void;

FOCP_END();

#endif
