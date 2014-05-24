
#include "DateTime.hpp"

#include <math.h>

#ifdef WINDOWS
#include <windows.h>
#else
#include <sys/time.h>
#include <time.h>
#endif

FOCP_BEGIN();

uint32 g_pMonthDays[2][12] =
{
	{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
	{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};

ADT_API bool IsLeapYear(uint16 Y)
{
	return ((Y%4) == 0) && (((Y% 100) != 0) || ((Y%400) == 0));
}

ADT_API bool EncodeTime(int32 &T, uint16 h, uint16 m, uint16 s, uint16 ms)
{
	bool bRet = false;
	if( (h<24) && (m<60) && (s<60) && (ms<1000) )
	{
		T = h*3600000 + m*60000 + s*1000 + ms;
		bRet = true;
	}
	return bRet;
}

ADT_API bool DecodeTime(int32 T, uint16 &h, uint16 &m, uint16 &s, uint16 &ms)
{
	if(T < 0 || T >= FOCP_MAX_TIME)
		return false;
	ms = (T % 1000);
	T /= 1000;
	s = (T % 60);
	T /= 60;
	m = (T % 60);
	T /= 60;
	h = T;
	return true;
}

ADT_API bool EncodeDate(int32 &D, uint16 y, uint16 m, uint16 d)
{
	bool bRet = false;
	bool bLeap = IsLeapYear(y);
	uint32 * pDayTab = g_pMonthDays[bLeap?1:0];
	if( (y >=1 && y <= 9999) && (m >= 1 && m <= 12) && (d >= 1 && d <= pDayTab[m-1]))
	{
		uint32 i;
		for(i=1; i<m; ++i)
			d += pDayTab[i-1];
		i = y - 1;
		D = i*365+i/4-i/100+i/400+d;
		bRet = true;
	}
	return bRet;
}

enum
{
	D1 = 365,
	D4 = D1 * 4 + 1,
	D100 = D4 * 25 - 1,
	D400 = D100 * 4 + 1,
};

ADT_API bool DecodeDate(int32 D, uint16 &y, uint16 &m, uint16 &d)
{
	int32 x;
	uint16 I;
	uint32* pDayTab;

	if(D < 1 || D > FOCP_MAX_DATE)
		return false;
	--D;
	y = 1;
//	while(D >= D400)
//	{
//		D -= D400;
//		y += 400;
//	}
	x = D / D400;
	if(x)
	{
		D %= D400;
		y += x * 400;
	}
	I = D / D100;
	d = D % D100;
	if(I == 4)
	{
		--I;
		d += D100;
	}
	y += I * 100;
	I = d / D4;
	d %= D4;
	y += I * 4;
	I = d / D1;
	d %= D1;
	if(I == 4)
	{
		--I;
		d += D1;
	}
	y += I;
	pDayTab = g_pMonthDays[IsLeapYear(y)?1:0];
	m = 1;
	while(true)
	{
		I = pDayTab[m-1];
		if(d < I)
			break;
		d -= I;
		++m;
	}
	++d;
	return true;
}

ADT_API bool EncodeDateTime(double &DT, int32 D, int32 T)
{
	if(D < 1 || D > FOCP_MAX_DATE)
		return false;
	if(T < 0 || T >= FOCP_MAX_TIME)
		return false;
	DT = T;
	DT /= FOCP_MAX_TIME;
	DT += D - 693594;
	return true;
}

static double FOCP_MIN_DATETIME = -693593, FOCP_MAX_DATETIME = 2958465 + 86399999.0/FOCP_MAX_TIME;

ADT_API bool DecodeDateTime(double DT, int32 &D, int32 &T)
{
	if(DT < FOCP_MIN_DATETIME || DT > FOCP_MAX_DATETIME)
		return false;
	DT += 693594;
	D = (int32)DT;
	DT -= D;
	DT *= FOCP_MAX_TIME;
	T = (int32)(DT+0.5);
	return true;
}

CDate::~CDate()
{
}

CDate::CDate(int32 nVal)
{
	m_nVal = nVal % FOCP_MAX_DATE;
	if(m_nVal < FOCP_MIN_DATE)
		m_nVal += FOCP_MAX_DATE;
}

CDate::CDate(CFormatter &oFmt)
{
	uint32 Y, M, D;
	int32 nRet = oFmt.Scan("%4d-%2d-%2d", &Y,&M,&D);
	if(nRet != 3 || !EncodeDate(m_nVal, (uint16)Y, (uint16)M, (uint16)D))
		m_nVal = FOCP_MIN_DATE;
}

CDate::CDate(const CString &oDateStr)
{
	uint32 Y, M, D;
	CStringFormatter oFmt((CString*)&oDateStr);
	int32 nRet = oFmt.Scan("%4d-%2d-%2d", &Y,&M,&D);
	if(nRet != 3 || !EncodeDate(m_nVal, (uint16)Y, (uint16)M, (uint16)D))
		m_nVal = FOCP_MIN_DATE;
}

CDate::CDate(uint16 Y, uint16 M, uint16 D)
{
	if(!EncodeDate(m_nVal, Y, M, D))
		m_nVal = FOCP_MIN_DATE;
}

CDate::CDate(const CDate& oDate):m_nVal(oDate.m_nVal)
{
}

CDate& CDate::operator=(const CDate& oDate)
{
	m_nVal = oDate.m_nVal;
	return *this;
}

CDate& CDate::operator=(int32 nVal)
{
	m_nVal = nVal % FOCP_MAX_DATE;
	if(m_nVal < FOCP_MIN_DATE)
		m_nVal += FOCP_MAX_DATE;
	else if(m_nVal > FOCP_MAX_DATE)
		m_nVal -= FOCP_MAX_DATE;
	return *this;
}

int32 CDate::GetValue() const
{
	return m_nVal;
}

CDate& CDate::Today()
{
#ifdef WINDOWS
	SYSTEMTIME nSystemTime;
	GetLocalTime(&nSystemTime);
	EncodeDate(m_nVal, nSystemTime.wYear, nSystemTime.wMonth, nSystemTime.wDay);
#else
	struct tm oTm;
	time_t oTime;
	time(&oTime);
	localtime_r(&oTime, &oTm);
	EncodeDate(m_nVal, oTm.tm_year + 1900, oTm.tm_mon + 1, oTm.tm_mday);
#endif
	return *this;
}

CDate& CDate::IncYear(int16 nYear)
{
	if(nYear)
	{
		uint16 Y,M,D;
		DecodeDate(m_nVal,Y,M,D);
		int32 nNewYear = nYear + Y;
		if( nNewYear < 1 || nNewYear > 9999)
			nNewYear = ((uint32)nNewYear % 9999) + 1;
		Y = (uint16)nNewYear;
		if(M == 2 && !IsLeapYear(Y) && D==29)
			D = 28;
		EncodeDate(m_nVal,Y,M,D);
	}
	return *this;
}

CDate& CDate::IncMonth(int16 nMonth)
{
	if(nMonth)
	{
		uint16 Y,M,D;
		int16 nYear = nMonth / 12;
		if(nYear)
			nMonth %= 12;
		DecodeDate(m_nVal,Y,M,D);
		nMonth += M;
		if(nMonth > 12)
		{
			nMonth -= 12;
			++nYear;
		}
		else if(nMonth < 1)
		{
			nMonth += 12;
			--nYear;
		}
		if(nYear)
		{
			int32 nNewYear = nYear + Y;
			if( nNewYear < 1 || nNewYear > 9999)
				nNewYear = ((uint32)nNewYear % 9999) + 1;
			Y = (uint16)nNewYear;
		}
		if(nYear || M != nMonth)
		{
			M = nMonth;
			bool bLeap = IsLeapYear(Y);
			uint32 nFOCP_MAX_DATE = g_pMonthDays[bLeap?1:0][M-1];
			if(D > nFOCP_MAX_DATE)
				D = nFOCP_MAX_DATE;
			EncodeDate(m_nVal,Y,M,D);
		}
	}
	return *this;
}

CDate& CDate::IncDay(int16 nDay)
{
	if(nDay)
	{
		m_nVal += nDay;
		if(m_nVal < FOCP_MIN_DATE)
			m_nVal += FOCP_MAX_DATE;
		else if(m_nVal > FOCP_MAX_DATE)
			m_nVal -= FOCP_MAX_DATE;
	}
	return *this;
}

CDate& CDate::operator+=(const CDate& oSrc)
{
	m_nVal += oSrc.m_nVal;
	if(m_nVal > FOCP_MAX_DATE)
		m_nVal -= FOCP_MAX_DATE;
	return *this;
}

CDate& CDate::operator-=(const CDate& oSrc)
{
	m_nVal -= oSrc.m_nVal;
	if(m_nVal < FOCP_MIN_DATE)
		m_nVal += FOCP_MAX_DATE;
	return *this;
}

CDate& CDate::operator++()
{
	++m_nVal;
	if(m_nVal > FOCP_MAX_DATE)
		m_nVal -= FOCP_MAX_DATE;
	return *this;
}

CDate& CDate::operator--()
{
	--m_nVal;
	if(m_nVal < FOCP_MIN_DATE)
		m_nVal += FOCP_MAX_DATE;
	return *this;
}

CDate CDate::operator++(int)
{
	CDate oTmp(m_nVal);
	++m_nVal;
	if(m_nVal > FOCP_MAX_DATE)
		m_nVal -= FOCP_MAX_DATE;
	return oTmp;
}

CDate CDate::operator--(int)
{
	CDate oTmp(m_nVal);
	--m_nVal;
	if(m_nVal < FOCP_MIN_DATE)
		m_nVal += FOCP_MAX_DATE;
	return oTmp;
}

CDate CDate::operator+(const CDate& oSrc) const
{
	int32 nVal = m_nVal + oSrc.m_nVal;
	if(nVal > FOCP_MAX_DATE)
		nVal -= FOCP_MAX_DATE;
	return nVal;
}

CDate CDate::operator-(const CDate& oSrc) const
{
	int32 nVal = m_nVal - oSrc.m_nVal;
	if(nVal < FOCP_MIN_DATE)
		nVal += FOCP_MAX_DATE;
	return nVal;
}

bool CDate::operator ==(const CDate& rhs) const
{
	return m_nVal == rhs.m_nVal;
}

bool CDate::operator !=(const CDate& rhs) const
{
	return m_nVal != rhs.m_nVal;
}

bool CDate::operator >(const CDate& rhs) const
{
	return m_nVal > rhs.m_nVal;
}

bool CDate::operator <(const CDate& rhs) const
{
	return m_nVal < rhs.m_nVal;
}

bool CDate::operator >=(const CDate& rhs) const
{
	return m_nVal >= rhs.m_nVal;
}

bool CDate::operator <=(const CDate& rhs) const
{
	return m_nVal <= rhs.m_nVal;
}

int32 CDate::DayOfWeek() const
{
	return m_nVal % 7 + 1;
}

void CDate::GetDate(uint16 &Y, uint16 &M, uint16 &D) const
{
	DecodeDate(m_nVal, Y, M, D);
}

void CDate::Print(CFormatter &oFmt) const
{
	uint16 Y,M,D;
	DecodeDate(m_nVal, Y, M, D);
	oFmt.Print("%04d-%02d-%02d", (uint32)Y, (uint32)M, (uint32)D);
}

bool CDate::Scan(CFormatter &oFmt)
{
	uint32 Y, M, D;
	int32 nRet = oFmt.Scan("%4d-%2d-%2d", &Y,&M,&D);
	if(nRet != 3 || !EncodeDate(m_nVal, (uint16)Y, (uint16)M, (uint16)D))
		return false;
	return true;
}

void CDate::GetString(CString &oDateStr) const
{
	CStringFormatter oFmt(&oDateStr);
	Print(oFmt);
}

CTime::~CTime()
{
}

CTime::CTime(int32 nVal)
{
	m_nVal = nVal % FOCP_MAX_TIME;
	if(m_nVal < 0)
		m_nVal += FOCP_MAX_TIME;
}

CTime::CTime(CFormatter &oFmt)
{
	uint32 H,M,S,MS=0;
	int32 nRet = oFmt.Scan("%2d:%2d:%2d.%3d", &H, &M, &S, &MS);
	if(nRet < 3 && !EncodeTime(m_nVal, (uint16)H, (uint16)M, (uint16)S, (uint16)MS))
		m_nVal = 0;
}

CTime::CTime(const CString &oTimeStr)
{
	uint32 H,M,S,MS=0;
	CStringFormatter oFmt((CString*)&oTimeStr);
	int32 nRet = oFmt.Scan("%2d:%2d:%2d.%3d", &H, &M, &S, &MS);
	if(nRet < 3 && !EncodeTime(m_nVal, (uint16)H, (uint16)M, (uint16)S, (uint16)MS))
		m_nVal = 0;
}

CTime::CTime(uint16 H, uint16 M, uint16 S, uint16 MS)
{
	if(!EncodeTime(m_nVal, H, M, S, MS))
		m_nVal = 0;
}

CTime::CTime(const CTime& oTime):m_nVal(oTime.m_nVal)
{
}

CTime& CTime::operator=(const CTime& oTime)
{
	if(this != &oTime)
		m_nVal = oTime.m_nVal;
	return *this;
}

CTime& CTime::operator=(int32 nVal)
{
	m_nVal = nVal % FOCP_MAX_TIME;
	if(m_nVal < 0)
		m_nVal += FOCP_MAX_TIME;
	return *this;
}

int32 CTime::GetValue() const
{
	return m_nVal;
}

CTime& CTime::Now()
{
#ifdef WINDOWS
	SYSTEMTIME nSystemTime;
	GetLocalTime(&nSystemTime);
	EncodeTime(m_nVal, nSystemTime.wHour, nSystemTime.wMinute, nSystemTime.wSecond, nSystemTime.wMilliseconds);
#else
	struct tm oTm;
	time_t oTime;
	struct timeval oTV;
	gettimeofday(&oTV, NULL);
	oTime = oTV.tv_sec;
	localtime_r(&oTime, &oTm);
	EncodeTime(m_nVal, oTm.tm_hour, oTm.tm_min, oTm.tm_sec, oTV.tv_usec/1000);
#endif
	return *this;
}

CTime& CTime::IncHour(int16 nHour)
{
	nHour %= 24;
	if(nHour)
		m_nVal = (m_nVal + nHour * 3600000 + FOCP_MAX_TIME) % FOCP_MAX_TIME;
	return *this;
}

CTime& CTime::IncMinute(int16 nMinute)
{
	nMinute %= 1440;
	if(nMinute)
		m_nVal = (m_nVal + nMinute*60000 + FOCP_MAX_TIME) % FOCP_MAX_TIME;
	return *this;
}

CTime& CTime::IncSecond(int16 nSecond)
{
	if(nSecond)
		m_nVal = (m_nVal + nSecond*1000 + FOCP_MAX_TIME) % FOCP_MAX_TIME;
	return *this;
}

CTime& CTime::IncMilliSecond(int16 nMilliSecond)
{
	if(nMilliSecond)
		m_nVal = (m_nVal + nMilliSecond + FOCP_MAX_TIME) % FOCP_MAX_TIME;
	return *this;
}

CTime& CTime::operator+=(const CTime& oSrc)
{
	m_nVal = (m_nVal + oSrc.m_nVal) % FOCP_MAX_TIME;
	return *this;
}

CTime& CTime::operator-=(const CTime& oSrc)
{
	m_nVal = (m_nVal - oSrc.m_nVal + FOCP_MAX_TIME) % FOCP_MAX_TIME;
	return *this;
}

CTime& CTime::operator++()
{
	m_nVal = (m_nVal + 1) % FOCP_MAX_TIME;
	return *this;
}

CTime CTime::operator++(int)
{
	CTime oTmp(m_nVal);
	m_nVal = (m_nVal + 1) % FOCP_MAX_TIME;
	return oTmp;
}

CTime& CTime::operator--()
{
	m_nVal = (m_nVal - 1 + FOCP_MAX_TIME) % FOCP_MAX_TIME;
	return *this;
}

CTime CTime::operator--(int)
{
	CTime oTmp(m_nVal);
	m_nVal = (m_nVal - 1 + FOCP_MAX_TIME) % FOCP_MAX_TIME;
	return oTmp;
}

CTime CTime::operator+(const CTime& oSrc) const
{
	return CTime((m_nVal+oSrc.m_nVal)%FOCP_MAX_TIME);
}

CTime CTime::operator-(const CTime& oSrc) const
{
	return CTime((m_nVal-oSrc.m_nVal+FOCP_MAX_TIME)%FOCP_MAX_TIME);
}

bool CTime::operator ==(const CTime& rhs) const
{
	return m_nVal == rhs.m_nVal;
}

bool CTime::operator !=(const CTime& rhs) const
{
	return m_nVal != rhs.m_nVal;
}

bool CTime::operator >(const CTime& rhs) const
{
	return m_nVal > rhs.m_nVal;
}

bool CTime::operator <(const CTime& rhs) const
{
	return m_nVal < rhs.m_nVal;
}

bool CTime::operator >=(const CTime& rhs) const
{
	return m_nVal >= rhs.m_nVal;
}

bool CTime::operator <=(const CTime& rhs) const
{
	return m_nVal <= rhs.m_nVal;
}

void CTime::GetTime(uint16 &H, uint16 &M, uint16 &S, uint16 &MS) const
{
	DecodeTime(m_nVal, H, M, S, MS);
}

void CTime::Print(CFormatter &oFmt) const
{
	uint16 H,M,S,MS;
	DecodeTime(m_nVal, H, M, S, MS);
	if(MS)
		oFmt.Print("%02d:%02d:%02d.%03d", (uint32)H, (uint32)M, (uint32)S, (uint32)MS);
	else
		oFmt.Print("%02d:%02d:%02d", (uint32)H, (uint32)M, (uint32)S);
}

bool CTime::Scan(CFormatter &oFmt)
{
	uint32 H,M,S,MS=0;
	int32 nRet = oFmt.Scan("%2d:%2d:%2d.%3d", &H, &M, &S, &MS);
	if(nRet < 3 && !EncodeTime(m_nVal, (uint16)H, (uint16)M, (uint16)S, (uint16)MS))
		return false;
	return true;
}

void CTime::GetString(CString &oTimeStr)
{
	CStringFormatter oFmt(&oTimeStr);
	Print(oFmt);
}

CDateTime::~CDateTime()
{
}

CDateTime::CDateTime(double nVal)
{
	m_nVal = fmod(nVal+693594, (double)FOCP_MAX_DATE) - 693594;
	if(m_nVal < FOCP_MIN_DATETIME)
		m_nVal += FOCP_MAX_DATE;
}

CDateTime::CDateTime(CFormatter &oFmt)
{
	int32 d, t;
	uint32 Y,M,D,h,m,s,ms=0;
	int32 nRet = oFmt.Scan("%4d-%2d-%2d %2d:%2d:%2d.%3d", &Y, &M, &D, &h, &m, &s, &ms);
	if(nRet < 6 || !EncodeDate(d, (uint16)Y, (uint16)M, (uint16)D) ||
			!EncodeTime(t, (uint16)h, (uint16)m, (uint16)s, (uint16)ms))
		m_nVal = 0;
	else
		EncodeDateTime(m_nVal, d, t);
}

CDateTime::CDateTime(const CString &oDateTimeStr)//YYYY-MM-DD HH:MM:SS[.MS]
{
	int32 d, t;
	uint32 Y,M,D,h,m,s,ms=0;
	CStringFormatter oFmt((CString*)&oDateTimeStr);
	int32 nRet = oFmt.Scan("%4d-%2d-%2d %2d:%2d:%2d.%3d", &Y, &M, &D, &h, &m, &s, &ms);
	if(nRet < 6 || !EncodeDate(d, (uint16)Y, (uint16)M, (uint16)D) ||
			!EncodeTime(t, (uint16)h, (uint16)m, (uint16)s, (uint16)ms))
		m_nVal = 0;
	else
		EncodeDateTime(m_nVal, d, t);
}

CDateTime::CDateTime(const CDate& oDate, const CTime& oTime)
{
	EncodeDateTime(m_nVal, oDate.m_nVal, oTime.m_nVal);
}

CDateTime::CDateTime(uint16 Y, uint16 M, uint16 D, uint16 h, uint16 m, uint16 s, uint16 ms)
{
	EncodeDateTime(m_nVal, CDate(Y,M,D).m_nVal, CTime(h,m,s,ms).m_nVal);
}

CDateTime::CDateTime(const CDateTime& oDateTime):m_nVal(oDateTime.m_nVal)
{
}

CDateTime& CDateTime::operator=(const CDateTime& oDateTime)
{
	m_nVal = oDateTime.m_nVal;
	return *this;
}

CDateTime& CDateTime::operator=(double nVal)
{
	m_nVal = fmod(nVal+693594, (double)FOCP_MAX_DATE) - 693594;
	if(m_nVal < FOCP_MIN_DATETIME)
		m_nVal += FOCP_MAX_DATE;
	return *this;
}

double CDateTime::GetValue() const
{
	return m_nVal;
}

CDate CDateTime::GetDate() const
{
	int32 d, t;
	DecodeDateTime(m_nVal, d, t);
	return d;
}

void CDateTime::SetDate(const CDate& oDate)
{
	int32 d, t;
	DecodeDateTime(m_nVal, d, t);
	EncodeDateTime(m_nVal, oDate.m_nVal, t);
}

CTime CDateTime::GetTime() const
{
	int32 d, t;
	DecodeDateTime(m_nVal, d, t);
	return t;
}

void CDateTime::SetTime(const CTime& oTime)
{
	int32 d, t;
	DecodeDateTime(m_nVal, d, t);
	EncodeDateTime(m_nVal, d, oTime.m_nVal);
}

void CDateTime::GetDateTime(uint16 &Y, uint16 &M, uint16 &D, uint16 &h, uint16 &m, uint16 &s, uint16 &ms) const
{
	int32 d, t;
	DecodeDateTime(m_nVal, d, t);
	DecodeDate(d, Y, M, D);
	DecodeTime(t, h, m, s, ms);
}

void CDateTime::Print(CFormatter &oFmt) const
{
	uint16 Y,M,D,h,m,s,ms;
	GetDateTime(Y,M,D,h,m,s,ms);
	if(ms)
		oFmt.Print("%04d-%02d-%02d %02d:%02d:%02d.%03d", (uint32)Y, (uint32)M, (uint32)D, (uint32)h, (uint32)m, (uint32)s, (uint32)ms);
	else
		oFmt.Print("%04d-%02d-%02d %02d:%02d:%02d", (uint32)Y, (uint32)M, (uint32)D, (uint32)h, (uint32)m, (uint32)s);
}

bool CDateTime::Scan(CFormatter &oFmt)
{
	int32 d, t;
	uint32 Y,M,D,h,m,s,ms=0;
	int32 nRet = oFmt.Scan("%4d-%2d-%2d %2d:%2d:%2d.%3d", &Y, &M, &D, &h, &m, &s, &ms);
	if(nRet < 6 || !EncodeDate(d, (uint16)Y, (uint16)M, (uint16)D) ||
			!EncodeTime(t, (uint16)h, (uint16)m, (uint16)s, (uint16)ms))
		return false;
	EncodeDateTime(m_nVal, d, t);
	return true;
}

void CDateTime::GetString(CString &oDateTimeStr)
{
	CStringFormatter oFmt(&oDateTimeStr);
	Print(oFmt);
}

CDateTime& CDateTime::Now()
{
	EncodeDateTime(m_nVal, CDate().Today().m_nVal, CTime().Now().m_nVal);
	return *this;
}

CDateTime& CDateTime::IncYear(int16 nYear)
{
	int32 d, t;
	if(nYear)
	{
		DecodeDateTime(m_nVal, d, t);
		EncodeDateTime(m_nVal, CDate(d).IncYear(nYear).m_nVal, t);
	}
	return *this;
}

CDateTime& CDateTime::IncMonth(int16 nMonth)
{
	int32 d, t;
	if(nMonth)
	{
		DecodeDateTime(m_nVal, d, t);
		EncodeDateTime(m_nVal, CDate(d).IncMonth(nMonth).m_nVal, t);
	}
	return *this;
}

CDateTime& CDateTime::IncDay(int16 nDay)
{
	if(nDay)
	{
		m_nVal += nDay;
		if(m_nVal < FOCP_MIN_DATETIME)
			m_nVal += FOCP_MAX_DATE;
		else if(m_nVal > FOCP_MAX_DATETIME)
			m_nVal -= FOCP_MAX_DATE;
	}
	return *this;
}

CDateTime& CDateTime::IncHour(int16 nHour)
{
	if(nHour)
	{
		m_nVal += nHour/24.0;
		if(m_nVal < FOCP_MIN_DATETIME)
			m_nVal += FOCP_MAX_DATE;
		else if(m_nVal > FOCP_MAX_DATETIME)
			m_nVal -= FOCP_MAX_DATE;
	}
	return *this;
}

CDateTime& CDateTime::IncMinute(int16 nMinute)
{
	if(nMinute)
	{
		m_nVal += nMinute/1440.0;
		if(m_nVal < FOCP_MIN_DATETIME)
			m_nVal += FOCP_MAX_DATE;
		else if(m_nVal > FOCP_MAX_DATETIME)
			m_nVal -= FOCP_MAX_DATE;
	}
	return *this;
}

CDateTime& CDateTime::IncSecond(int16 nSecond)
{
	if(nSecond)
	{
		m_nVal += nSecond/86400.0;
		if(m_nVal < FOCP_MIN_DATETIME)
			m_nVal += FOCP_MAX_DATE;
		else if(m_nVal > FOCP_MAX_DATETIME)
			m_nVal -= FOCP_MAX_DATE;
	}
	return *this;
}

CDateTime& CDateTime::IncMilliSecond(int16 nMilliSecond)
{
	if(nMilliSecond)
	{
		m_nVal += nMilliSecond/86400000.0;
		if(m_nVal < FOCP_MIN_DATETIME)
			m_nVal += FOCP_MAX_DATE;
		else if(m_nVal > FOCP_MAX_DATETIME)
			m_nVal -= FOCP_MAX_DATE;
	}
	return *this;
}

CDateTime& CDateTime::operator+=(const CDateTime& oSrc)
{
	m_nVal += oSrc.m_nVal;
	if(m_nVal > FOCP_MAX_DATETIME)
		m_nVal -= FOCP_MAX_DATE;
	return *this;
}

CDateTime& CDateTime::operator-=(const CDateTime& oSrc)
{
	m_nVal -= oSrc.m_nVal;
	if(m_nVal < FOCP_MIN_DATETIME)
		m_nVal += FOCP_MAX_DATE;
	return *this;
}

CDateTime& CDateTime::operator++()
{
	++m_nVal;
	if(m_nVal > FOCP_MAX_DATETIME)
		m_nVal -= FOCP_MAX_DATE;
	return *this;
}

CDateTime CDateTime::operator++(int)
{
	CDateTime oTmp(*this);
	++m_nVal;
	if(m_nVal > FOCP_MAX_DATETIME)
		m_nVal -= FOCP_MAX_DATE;
	return oTmp;
}

CDateTime& CDateTime::operator--()
{
	--m_nVal;
	if(m_nVal < FOCP_MIN_DATETIME)
		m_nVal += FOCP_MAX_DATE;
	return *this;
}

CDateTime CDateTime::operator--(int)
{
	CDateTime oTmp(*this);
	--m_nVal;
	if(m_nVal < FOCP_MIN_DATETIME)
		m_nVal += FOCP_MAX_DATE;
	return oTmp;
}

CDateTime CDateTime::operator+(const CDateTime& oSrc) const
{
	CDateTime oTmp;
	oTmp.m_nVal = m_nVal + oSrc.m_nVal;
	if(oTmp.m_nVal > FOCP_MAX_DATETIME)
		oTmp.m_nVal -= FOCP_MAX_DATE;
	return oTmp;
}

CDateTime CDateTime::operator-(const CDateTime& oSrc) const
{
	CDateTime oTmp;
	oTmp.m_nVal = m_nVal - oSrc.m_nVal;
	if(oTmp.m_nVal < FOCP_MIN_DATETIME)
		oTmp.m_nVal += FOCP_MAX_DATE;
	return oTmp;
}

bool CDateTime::operator ==(const CDateTime& rhs) const
{
	return m_nVal == rhs.m_nVal;
}

bool CDateTime::operator !=(const CDateTime& rhs) const
{
	return m_nVal != rhs.m_nVal;
}

bool CDateTime::operator >(const CDateTime& rhs) const
{
	return m_nVal > rhs.m_nVal;
}

bool CDateTime::operator <(const CDateTime& rhs) const
{
	return m_nVal < rhs.m_nVal;
}

bool CDateTime::operator >=(const CDateTime& rhs) const
{
	return m_nVal >= rhs.m_nVal;
}

bool CDateTime::operator <=(const CDateTime& rhs) const
{
	return m_nVal <= rhs.m_nVal;
}
void CDateTime::SetFileDate(int32 nFileDate)
{
#ifdef WINDOWS
	uint16 nHi = nFileDate>>16;
	uint16 nLo = nFileDate&0xFFFF;
	EncodeDateTime(m_nVal, CDate((nHi>>9)+1980, (nHi>>5)&15, nHi&31).m_nVal,
				   CTime(nLo>>11, (nLo>>5)&63, (nLo&31)>>1, 0).m_nVal);
#else
	struct tm oTm;
	localtime_r((time_t*)&nFileDate, &oTm);
	EncodeDateTime(m_nVal, CDate(oTm.tm_year + 1900, oTm.tm_mon + 1, oTm.tm_mday).m_nVal,
				   CTime(oTm.tm_hour, oTm.tm_min, oTm.tm_sec, 0).m_nVal);
#endif
}

int32 CDateTime::GetFileDate()  const
{
	uint16 Y, M, D, h, m, s, ms;
	GetDateTime(Y, M, D, h, m, s, ms);
#ifdef WINDOWS
	if(Y < 1980 || Y > 2107)
		return 0;
	uint16 nLo = ((s > 1) | (m << 5) | (h << 11));
	uint16 nHi = D | (M << 5) | ((Y-1980)<<9);
	return ((uint32)nHi<<16)|nLo;
#else
	struct tm T;
	if (Y < 1970 || Y > 2038)
		return 0;
	T.tm_sec = s;
	T.tm_min = m;
	T.tm_hour = h;
	T.tm_mday = D;
	T.tm_mon  = M - 1;
	T.tm_year = Y - 1900;
	T.tm_wday = 0;
	T.tm_yday = 0;
	T.tm_isdst = -1;
	return mktime(&T);
#endif
}

FOCP_END();
