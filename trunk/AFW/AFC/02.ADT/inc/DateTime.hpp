
#include "Formatter.hpp"

#ifndef _ADT_DATETIME_HPP_
#define _ADT_DATETIME_HPP_

FOCP_BEGIN();

ADT_API bool IsLeapYear(uint16 Y);
ADT_API bool EncodeTime(int32 &T, uint16 h, uint16 m, uint16 s, uint16 ms);
ADT_API bool DecodeTime(int32 T, uint16 &h, uint16 &m, uint16 &s, uint16 &ms);
ADT_API bool EncodeDate(int32 &D, uint16 y, uint16 m, uint16 d);
ADT_API bool DecodeDate(int32 D, uint16 &y, uint16 &m, uint16 &d);
ADT_API bool EncodeDateTime(double &DT, int32 D, int32 T);
ADT_API bool DecodeDateTime(double DT, int32 &D, int32 &T);

enum
{
	FOCP_MIN_DATE = 1,
	FOCP_MAX_DATE = 3652059,//[FOCP_MIN_DATE, FOCP_MAX_DATE]
	FOCP_MIN_TIME = 0,
	FOCP_MAX_TIME = 86400000,//[FOCP_MIN_TIME, FOCP_MAX_TIME)
};

/*
	注意CDate CTime CDateTime不能使用虚函数
*/
class ADT_API CDate
{
	friend class CDateTime;
private:
	int32 m_nVal;

public:
	~CDate();
	CDate(int32 nVal=FOCP_MIN_DATE);
	CDate(CFormatter &oFmt);
	CDate(const CString &oDateStr);//YYYY-MM-DD
	CDate(uint16 Y, uint16 M, uint16 D);
	CDate(const CDate& oDate);
	CDate& operator=(const CDate& oDate);
	CDate& operator=(int32 nVal);

	int32 GetValue() const;

	CDate& Today();
	CDate& IncYear(int16 nYear);
	CDate& IncMonth(int16 nMonth);
	CDate& IncDay(int16 nDay);

	CDate& operator+=(const CDate& oSrc);
	CDate& operator-=(const CDate& oSrc);
	CDate& operator++();
	CDate& operator--();
	CDate operator++(int);
	CDate operator--(int);
	CDate operator+(const CDate& oSrc) const;
	CDate operator-(const CDate& oSrc) const;

	bool operator ==(const CDate& rhs) const;
	bool operator !=(const CDate& rhs) const;
	bool operator >(const CDate& rhs) const;
	bool operator <(const CDate& rhs) const;
	bool operator >=(const CDate& rhs) const;
	bool operator <=(const CDate& rhs) const;

	int32 DayOfWeek() const;
	void GetDate(uint16 &Y, uint16 &M, uint16 &D) const;
	void GetString(CString &oDateStr) const;
	void Print(CFormatter &oFmt) const;
	bool Scan(CFormatter &oFmt);
};

class ADT_API CTime
{
	friend class CDateTime;
private:
	int32 m_nVal;

public:
	~CTime();
	CTime(int32 nVal = FOCP_MIN_TIME);
	CTime(CFormatter &oFmt);
	CTime(const CString &oTimeStr);//HH:MM:SS[.MS]
	CTime(uint16 H, uint16 M, uint16 S, uint16 MS);
	CTime(const CTime& oTime);
	CTime& operator=(const CTime& oTime);
	CTime& operator=(int32 nVal);

	int32 GetValue() const;

	CTime& Now();
	CTime& IncHour(int16 nHour);
	CTime& IncMinute(int16 nMinute);
	CTime& IncSecond(int16 nSecond);
	CTime& IncMilliSecond(int16 nMilliSecond);

	CTime& operator+=(const CTime& oSrc);
	CTime& operator-=(const CTime& oSrc);
	CTime& operator++();
	CTime operator++(int);
	CTime& operator--();
	CTime operator--(int);
	CTime operator+(const CTime& oSrc) const;
	CTime operator-(const CTime& oSrc) const;

	// comparisons
	bool operator ==(const CTime& rhs) const;
	bool operator !=(const CTime& rhs) const;
	bool operator >(const CTime& rhs) const;
	bool operator <(const CTime& rhs) const;
	bool operator >=(const CTime& rhs) const;
	bool operator <=(const CTime& rhs) const;

	void GetTime(uint16 &H, uint16 &M, uint16 &S, uint16 &MS) const;
	void GetString(CString &oTimeStr);
	void Print(CFormatter &oFmt) const;
	bool Scan(CFormatter &oFmt);
};

class ADT_API CDateTime
{
private:
	double m_nVal;

public:
	~CDateTime();
	CDateTime(double nVal=0);
	CDateTime(CFormatter &oFmt);
	CDateTime(const CString &oDateTimeStr);//YYYY-MM-DD HH:MM:SS[.MS]
	CDateTime(const CDate& oDate, const CTime& oTime);
	CDateTime(uint16 Y, uint16 M, uint16 D, uint16 h, uint16 m, uint16 s, uint16 ms);
	CDateTime(const CDateTime& oDateTime);
	CDateTime& operator=(const CDateTime& oDateTime);
	CDateTime& operator=(double nVal);

	double GetValue() const;

	CDate GetDate() const;
	void SetDate(const CDate& oDate);

	CTime GetTime() const;
	void SetTime(const CTime& oTime);

	void GetDateTime(uint16 &Y, uint16 &M, uint16 &D, uint16 &h, uint16 &m, uint16 &s, uint16 &ms) const;
	void GetString(CString &oDateTimeStr);
	void Print(CFormatter &oFmt) const;
	bool Scan(CFormatter &oFmt);

	CDateTime& Now();
	CDateTime& IncYear(int16 nYear);
	CDateTime& IncMonth(int16 nMonth);
	CDateTime& IncDay(int16 nDay);
	CDateTime& IncHour(int16 nHour);
	CDateTime& IncMinute(int16 nMinute);
	CDateTime& IncSecond(int16 nSecond);
	CDateTime& IncMilliSecond(int16 nMilliSecond);

	CDateTime& operator+=(const CDateTime& oSrc);
	CDateTime& operator-=(const CDateTime& oSrc);
	CDateTime& operator++();
	CDateTime operator++(int);
	CDateTime& operator--();
	CDateTime operator--(int);
	CDateTime operator+(const CDateTime& oSrc) const;
	CDateTime operator-(const CDateTime& oSrc) const;

	// comparisons
	bool operator ==(const CDateTime& rhs) const;
	bool operator !=(const CDateTime& rhs) const;
	bool operator >(const CDateTime& rhs) const;
	bool operator <(const CDateTime& rhs) const;
	bool operator >=(const CDateTime& rhs) const;
	bool operator <=(const CDateTime& rhs) const;

	int32 GetFileDate() const;
	void SetFileDate(int32 nFileDate);
};

FOCP_END();

#endif
