
#include "Any.hpp"

FOCP_BEGIN();

FOCP_DETAIL_BEGIN();

CAnyPlaceHolder::CAnyPlaceHolder()
{
	m_nType = 1;
}

CAnyPlaceHolder::~CAnyPlaceHolder()
{
}

CAnyPlaceHolder * CAnyPlaceHolder::Clone() const
{
	return NULL;
}

FOCP_DETAIL_END();

FOCP_END();
