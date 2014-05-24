
#ifndef _ASF_DEF_HPP_
#define _ASF_DEF_HPP_

#include "../../04.AFS/AFS.hpp"

#ifdef ASF_EXPORTS
#define ASF_API FOCP_EXPORT
#else
#define ASF_API FOCP_IMPORT
#endif

#define FOCP_RUNNER_UDPSERVICEPORT 1981 //UDP
#define FOCP_RUNNER_TCPSERVICEPORT 1982 //TCP

#endif
