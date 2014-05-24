
#ifndef _AFC_INTEGER_HPP_
#define _AFC_INTEGER_HPP_

#ifdef _MSC_VER

#if (_MSC_VER < 1300)
	#define FOCP_NO_TEMPLATE_PARTIAL_SPECIALIZATION
#endif

#endif

FOCP_BEGIN();

#ifdef FOCP_NO_TEMPLATE_PARTIAL_SPECIALIZATION
	//不支持类的偏特化的，往往支持局部类的全特化
	#define FOCP_FAKE_DEFAULT_TYPE(S)
	#define FOCP_FAKE_TYPE(S)
	#define FOCP_FAKE_TYPE_ARG(S)
#else
	//支持类的偏特化的，往往不支持局部类的全特化，所以需要做一个哑元缺省类型参数
	#define FOCP_FAKE_DEFAULT_TYPE(S) ,typename S=void
	#define FOCP_FAKE_TYPE(S) typename S
	#define FOCP_FAKE_TYPE_ARG(S) ,S
#endif

FOCP_DETAIL_BEGIN();

template<typename t, bool bSigned> struct CSignedInt
{
	typedef t xint;
};

#define FOCP_DEFINE_INTEGER_HELPER(n, t) \
	template<> struct CSignedInt<t, false> \
	{ \
		typedef signed t xint;\
	}; \
	template<> struct integerhelper<n> \
	{ \
	private: \
		enum{ii=(((t)(-1))<((t)(0)))}; \
		typedef CSignedInt<t, ii> xSignedInt; \
	public: \
		typedef xSignedInt::xint xint; \
		typedef unsigned t xuint; \
		typedef t int_t; \
		typedef signed t signed_int; \
		typedef unsigned t unsigned_int; \
	}

#ifdef MSVC
#define FOCP_LONGLONG __int64
#else
#define FOCP_LONGLONG long long
#endif

template<int i> struct integerhelper
{
	typedef int xint;
	typedef unsigned int xuint;
	typedef int int_t;
};

FOCP_DEFINE_INTEGER_HELPER(1, FOCP_LONGLONG);
FOCP_DEFINE_INTEGER_HELPER(2, long);
FOCP_DEFINE_INTEGER_HELPER(3, int);
FOCP_DEFINE_INTEGER_HELPER(4, short);
FOCP_DEFINE_INTEGER_HELPER(5, char);

template<int bits> struct integer
{
private:
	enum {i= ((bits>>3)<=sizeof(FOCP_LONGLONG)) +
			 ((bits>>3)<=sizeof(long)) +
			 ((bits>>3)<=sizeof(int)) +
			 ((bits>>3)<=sizeof(short)) +
			 ((bits>>3)<=sizeof(char))
		 };
	typedef integerhelper<i> subint;
public:
	typedef typename subint::xint xint;
	typedef typename subint::xuint xuint;
	typedef typename subint::int_t int_t;
	typedef typename subint::signed_int signed_int;
	typedef typename subint::unsigned_int unsigned_int;
};

enum {FOCP_SIZEOF_POINTER=sizeof(void*)};

#undef FOCP_DEFINE_INTEGER_HELPER
#undef FOCP_LONGLONG

FOCP_DETAIL_END();

typedef FOCP_DETAIL_NAME::integer<8>::xint int8;
typedef FOCP_DETAIL_NAME::integer<8>::xuint uint8;
typedef FOCP_DETAIL_NAME::integer<16>::xint int16;
typedef FOCP_DETAIL_NAME::integer<16>::xuint uint16;
typedef FOCP_DETAIL_NAME::integer<32>::xint int32;
typedef FOCP_DETAIL_NAME::integer<32>::xuint uint32;
typedef FOCP_DETAIL_NAME::integer<64>::xint int64;
typedef FOCP_DETAIL_NAME::integer<64>::xuint uint64;
typedef FOCP_DETAIL_NAME::integer<8*FOCP_DETAIL_NAME::FOCP_SIZEOF_POINTER>::xuint uintptr;
typedef unsigned long ulong;
typedef unsigned char uchar;

template<uint32 nSize, bool bSigned> struct CInteger
{
private:
	template<bool FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper
	{
		enum {nBits = nSize*8};
		typedef typename FOCP_DETAIL_NAME::integer<nBits>::signed_int xint;
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<false FOCP_FAKE_TYPE_ARG(S)>
	{
		enum {nBits = nSize*8};
		typedef typename FOCP_DETAIL_NAME::integer<nBits>::unsigned_int xint;
	};
public:
	typedef typename CHelper<bSigned>::xint xint;
};

static const uint8 FOCP_UINT8_MAX = (uint8)(int8)(-1);
static const int8 FOCP_INT8_MAX = FOCP_UINT8_MAX>>2;
static const int8 FOCP_INT8_MIN = ~FOCP_INT8_MAX;

static const uint16	FOCP_UINT16_MAX = (uint16)(int16)(-1);
static const int16 FOCP_INT16_MAX = FOCP_UINT16_MAX>>1;
static const int16 FOCP_INT16_MIN = ~FOCP_INT16_MAX;

static const uint32	FOCP_UINT32_MAX = (uint32)(int32)(-1);
static const int32 FOCP_INT32_MAX = FOCP_UINT32_MAX>>1;
static const int32 FOCP_INT32_MIN = ~FOCP_INT32_MAX;

static const uint64 FOCP_UINT64_MAX = (int64)(-1);
static const int64 FOCP_INT64_MAX = FOCP_UINT64_MAX>>1;
static const int64 FOCP_INT64_MIN = ~FOCP_INT64_MAX;

#ifdef MSVC
	#define FOCP_INT64_CONST(a) a##i64
	#define FOCP_UINT64_CONST(a) a##ui64
#else
	#define FOCP_INT64_CONST(a) a##ll
	#define FOCP_UINT64_CONST(a) a##ull
#endif

FOCP_END();

#endif //_AFC_INTEGER_HPP_
