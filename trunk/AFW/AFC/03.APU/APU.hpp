
#ifndef _APU_HPP_
#define _APU_HPP_

#include "./inc/Timer.hpp"
#include "./inc/Session.hpp"
#include "../02.ADT/ADT.hpp"

FOCP_BEGIN();

enum
{
	FOCP_FIBER_DEFAULT_GROUP = 0,
	FOCP_FIBER_TELNET_GROUP,
	FOCP_FIBER_LOGGR_GROUP,
};

enum
{
	FOCP_TELNET_LINK_FIBER = 1,
	FOCP_USER_FIBER=1048576,
};

FOCP_END();

#endif
