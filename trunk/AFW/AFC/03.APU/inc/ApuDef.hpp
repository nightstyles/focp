
#include "../../02.ADT/ADT.hpp"

#ifndef _APU_DEFINE_HPP_
#define _APU_DEFINE_HPP_

#ifdef APU_EXPORTS
#define APU_API FOCP_EXPORT
#else
#define APU_API FOCP_IMPORT
#endif

FOCP_BEGIN();

APU_API void EnterSystemRead();
APU_API void LeaveSystemRead();
APU_API void EnterSystemWrite();
APU_API void LeaveSystemWrite();

FOCP_END();

#endif
