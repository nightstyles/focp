
#include "../../03.APU/APU.hpp"

#ifndef _AFS_DEFINE_HPP_
#define _AFS_DEFINE_HPP_

#ifdef AFS_EXPORTS
#define AFS_API FOCP_EXPORT
#else
#define AFS_API FOCP_IMPORT
#endif

#define FOCP_LOGSERVER_PORT 1980 //UDP

#endif
