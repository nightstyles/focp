
#include "AdtDef.hpp"

#ifndef _Afc_Trait_Hpp_
#define _Afc_Trait_Hpp_

FOCP_BEGIN();

/****************************************************
 *删除指针/引用/常量修饰/变量修饰，可能需要根据编译器
 *的特征进行调整
*****************************************************/
#ifdef _MSC_VER

FOCP_DETAIL_BEGIN();
namespace Trait{
#if (_MSC_VER==1300)
template<typename ID> struct CBaseType
{
	template<bool> struct id2type_impl;
	typedef id2type_impl<true> id2type;
};
template<typename T, typename ID> struct CRegisterType : public CBaseType<ID>
{
	template<> struct id2type_impl<true>  //VC7.0 specific bugfeature
	{
		typedef T type;
	};
};
#elif (_MSC_VER>=1400)
struct CBaseTypeDefaultParam
{
};
template<typename ID, typename T = CBaseTypeDefaultParam> struct CBaseType;
template<typename ID> struct CBaseType<ID, CBaseTypeDefaultParam> 
{
	template<bool> struct id2type_impl;
	typedef id2type_impl<true> id2type;
};
template<typename ID, typename T> struct CBaseType : CBaseType<ID,CBaseTypeDefaultParam>
{
	template<> struct id2type_impl<true>  //VC8.0 specific bugfeature
	{
		typedef T type;
	};
	template<bool> struct id2type_impl;
	typedef id2type_impl<true> id2type;
};
template<typename T, typename ID> struct CRegisterType : CBaseType<ID, T>
{
};
#else
template<typename ID> struct CBaseType
{
	struct id2type;
};
template<typename T, typename ID> struct CRegisterType : public CBaseType<ID>
{
	typedef CBaseType<ID> TBaseType;
	struct TBaseType::id2type // This uses nice VC6.5 and VC7.1 bugfeature
	{
		typedef T type;
	};
};
#endif
}
FOCP_DETAIL_END();

#endif

/****************************************************
 *判断类型是否为指针
*****************************************************/
template<typename T> struct CIsPointer
{
private:
	template<typename U>static char Test(U const*);
	static short Test(...);
	static T MakeT();
public:
	enum {value = (sizeof(Test(MakeT()))==sizeof(char))};
};
template<> struct CIsPointer<void>
{
	enum {value = false};
};

/****************************************************
 *删除指针类型
*****************************************************/
#ifndef FOCP_NO_TEMPLATE_PARTIAL_SPECIALIZATION
template<typename T> struct CRemovePointer{typedef T type;};
template<typename T> struct CRemovePointer<T*>{typedef T type;};
template<typename T> struct CRemovePointer<T*const>{typedef T type;};
template<typename T> struct CRemovePointer<T*volatile>{typedef T type;};
template<typename T> struct CRemovePointer<T*const volatile>{typedef T type;};
#elif defined(_MSC_VER)

FOCP_DETAIL_BEGIN();
template<bool/*IsPointer*/> struct CRemovePointerHelper
{
	template<typename T,typename ID> struct inner
	{
		typedef T type;
	};
};

template<> struct CRemovePointerHelper<true>
{
	template<typename T,typename ID> struct inner
	{
		template<typename U> static Trait::CRegisterType<U,ID> test(U*);
		static Trait::CRegisterType<T,ID> test(...);
		enum {value=sizeof(test(*((T*)0)))};
		typedef typename Trait::CBaseType<ID>::id2type::type type;
	};
};
FOCP_DETAIL_END();

template<typename T> struct CRemovePointer
{
	typedef typename FOCP_DETAIL_NAME::CRemovePointerHelper<CIsPointer<T>::value>::inner< T, CRemovePointer<T> >::type type;
};

#endif

/****************************************************
 *判断类型是否为引用
*****************************************************/
template<typename T> struct CIsRefer
{
private:
	template<typename X> struct CWrap {};
	template<typename X> static X& (*(Test1)(CWrap<X>))(CWrap<X>);
	static uint8 Test1(...);
	template<typename X> static uint32 Test2(X&(*)(CWrap<X>));
	static uint8 Test2(...);
public:
	enum {value=sizeof(Test2(Test1(CWrap<T>())))==sizeof(uint8)};
};

/****************************************************
 *删除引用
*****************************************************/
#ifndef FOCP_NO_TEMPLATE_PARTIAL_SPECIALIZATION
template<typename T> struct CRemoveRefer{typedef T type;};
template<typename T> struct CRemoveRefer<T&>{typedef T type;};
//template<typename T> struct CRemoveRefer<T&const>{typedef T type;};
//template<typename T> struct CRemoveRefer<T&volatile>{typedef T type;};
//template<typename T> struct CRemoveRefer<T&const volatile>{typedef T type;};
#elif defined(_MSC_VER)
FOCP_DETAIL_BEGIN();
template<bool/*IsPointer*/> struct CRemoveReferHelper
{
	template<typename T,typename ID> struct inner
	{
		typedef T type;
	};
};

template<> struct CRemoveReferHelper<true>
{
	template<typename T,typename ID> struct inner
	{
		template<typename U> static Trait::CRegisterType<U,ID> test(U&(*)());
		static Trait::CRegisterType<T,ID> test(...);
		enum {value=sizeof(test((T(*)())0))};
		typedef typename Trait::CBaseType<ID>::id2type::type type;
	};
};
FOCP_DETAIL_END();
template<typename T> struct CRemoveRefer
{
	typedef typename FOCP_DETAIL_NAME::CRemoveReferHelper<CIsRefer<T>::value>::inner< T, CRemoveRefer<T> >::type type;
};
#endif

/****************************************************
 *判断两个类型是否一致
*****************************************************/
FOCP_DETAIL_BEGIN();
template <bool b1, bool b2> struct CIsSameTypeHelp
{
	template <typename T, typename U> struct CHelper
	{
	private:
		template<typename X> struct CPointer{X*p;};
		static char SameTest( const CPointer<U>* );
		static short SameTest(...);
	public:
		enum {value = (sizeof(SameTest((CPointer<T>*)NULL))==sizeof(char))};
	};
};
template <> struct CIsSameTypeHelp<true,false>
{
	template <typename T, typename U> struct CHelper
	{
		enum {value=false};
	};
};
template <> struct CIsSameTypeHelp<false,true>
{
	template <typename T, typename U> struct CHelper
	{
		enum {value=false};
	};
};
FOCP_DETAIL_END();
template <typename T, typename U> struct CIsSameType
{
	enum{value = FOCP_DETAIL_NAME::CIsSameTypeHelp<CIsRefer<T>::value, CIsRefer<U>::value>::
		template CHelper<typename CRemoveRefer<T>::type, typename CRemoveRefer<U>::type>::value};
};

/****************************************************
*	判断两个类型是否可转换，类型转换规则：
********************************************************/
template <typename T, typename U> struct CConvertible//继承转换
{
private:
	static uint8 Test(const U*);
	static uint16 Test(...);

public:
	enum {value = sizeof(Test((T*)0))==sizeof(uint8)};
};

template <typename T, typename U> struct CImplicitConvertible//可拷贝转换
{
private:
	static uint8 Test(const U);
	static uint16 Test(...);

public:
	enum {value = sizeof(Test(*(T*)0))==sizeof(uint8)};
};

template <typename T, typename U> struct CExplicitConvertible//拷贝构造
{
private:
	struct XX
	{
		explicit XX(const U&);
	};

	static char Test(const XX& x);
	static int Test(...);

public:
	enum {value = (sizeof(Test(*(T*)0))==sizeof(char))};
};

/****************************************************
	判断类型是否为聚合类型：struct, union, class
*****************************************************/
template<typename T> struct CIsUnionOrClass
{
private:
	template<class U> static uint8 Test(void(U::*)(void));
#if defined(_MSC_VER) && (_MSC_VER < 1300)
	static uint16 Test(...);
public:
	enum {value = sizeof(Test(0))==sizeof(uint8)};
#else
	template<class U> static uint16 Test(...);
public:
	enum {value = sizeof(Test<T>(0))==sizeof(uint8)};
#endif
};

/****************************************************
	判断两个是否为继承关系
*****************************************************/
template <typename TDerived, typename TBase> struct CIsInherited
{
	enum {value = CConvertible<TDerived,TBase>::value && !CIsSameType<TDerived,TBase>::value};
};

/****************************************************
 *判断是否为数组
*****************************************************/
template<typename T> struct CIsArray
{
private:
	template<typename U> struct CWrap {};
	template<typename X> static X (*Test1(CWrap<X>))(CWrap<X>);
	static uint8 Test1(...);
	template<typename X> static uint16 Test2(X(*)(CWrap<X>));
	static uint8 Test2(...);
public:
	enum {value=(sizeof(uint8)==sizeof(Test2(Test1(CWrap<T>()))))};
};

/****************************************************
 *删除数组类型
*****************************************************/
#ifndef FOCP_NO_TEMPLATE_PARTIAL_SPECIALIZATION
template<typename T> struct CRemoveArray{typedef T type;};
template<typename T,uint32 nSize> struct CRemoveArray<T[nSize]>{typedef T type;};
template<typename T,uint32 nSize> struct CRemoveArray<T const [nSize]>{typedef T const type;};
template<typename T,uint32 nSize> struct CRemoveArray<T volatile [nSize]>{typedef T volatile type;};
template<typename T,uint32 nSize> struct CRemoveArray<T const volatile [nSize]>{typedef T const volatile type;};
#elif defined(_MSC_VER)
template<typename T> struct CRemoveArray;
FOCP_DETAIL_BEGIN();
template<bool/*IsArray*/> struct CRemoveArrayHelper
{
	template<typename T,typename ID> struct inner
	{
		typedef T type;
	};
};

template<> struct CRemoveArrayHelper<true>
{
	template<typename T,typename ID> struct inner
	{
	private:
		template<typename U> static Trait::CRegisterType<U,ID> test(U[]);
		static Trait::CRegisterType<T,ID> test(...);
		enum {value=sizeof(test(*((T*)0)))};
	public:
		typedef typename Trait::CBaseType<ID>::id2type::type type;
	};
};
FOCP_DETAIL_END();

template<typename T> struct CRemoveArray
{
	typedef typename FOCP_DETAIL_NAME::CRemoveArrayHelper<CIsArray<T>::value>::inner< T, CRemoveArray<T> >::type type;
};
#endif

#ifndef FOCP_NO_TEMPLATE_PARTIAL_SPECIALIZATION
template<typename T> struct CRemoveArrays{typedef T type;};
template<typename T,uint32 nSize> struct CRemoveArrays<T[nSize]>{typedef typename CRemoveArrays<T>::type type;};
template<typename T,uint32 nSize> struct CRemoveArrays<T const [nSize]>{typedef typename CRemoveArrays<T const>::type type;};
template<typename T,uint32 nSize> struct CRemoveArrays<T volatile [nSize]>{typedef typename CRemoveArrays<T volatile>::type type;};
template<typename T,uint32 nSize> struct CRemoveArrays<T const volatile [nSize]>{typedef typename CRemoveArrays<T const volatile>::type type;};
#elif defined(_MSC_VER)
template<typename T> struct CRemoveArrays;
FOCP_DETAIL_BEGIN();
template<bool/*IsArray*/> struct CRemoveArraysHelper
{
	template<typename T,typename ID> struct inner
	{
		typedef T type;
	};
};

template<> struct CRemoveArraysHelper<true>
{
	template<typename T,typename ID> struct inner
	{
	private:
		template<typename U> static Trait::CRegisterType<U,ID> test(U[]);
		static Trait::CRegisterType<T,ID> test(...);
		enum {value=sizeof(test(*((T*)0)))};
		typedef typename Trait::CBaseType<ID>::id2type::type type0;
	public:
		typedef typename CRemoveArrays<type0>::type type;
	};
};
FOCP_DETAIL_END();
template<typename T> struct CRemoveArrays
{
	typedef typename FOCP_DETAIL_NAME::CRemoveArraysHelper<CIsArray<T>::value>::inner< T, CRemoveArrays<T> >::type type;
};
#endif

/****************************************************
 *获取数组的项数
*****************************************************/
template<typename T> struct CCountOfArray
{
private:
	template<bool FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper
	{
		enum {value=0};
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<true FOCP_FAKE_TYPE_ARG(S)>
	{
		enum {value = sizeof(T) / sizeof(CRemoveArray<T>::type)};
	};
public:
	enum {value=CHelper<CIsArray<T>::value>::value};
};

/****************************************************
 *判断是否有常量修饰
*****************************************************/
template<typename T> struct CIsConst
{
private:
	template<bool FOCP_FAKE_DEFAULT_TYPE(SS)> struct CHelper
	{
	private:
		template<bool FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelperA
		{
		private:
			static uint8 Test(const volatile void*);
			static uint16 Test(volatile void*);
		public:
			enum {value=sizeof(Test((T*)0))==sizeof(uint8)};
		};
		template<FOCP_FAKE_TYPE(S)> struct CHelperA<true FOCP_FAKE_TYPE_ARG(S)>
		{
		private:
			struct XX{ T t; };
			static uint8 Test(const volatile void*);
			static uint16 Test(volatile void*);
			static XX& MakeT();
		public:
			enum {value=sizeof(Test(&MakeT().t))==sizeof(uint8)};
		};
	public:
		enum {value=CHelperA<CIsArray<T>::value>::value};
	};
	template<FOCP_FAKE_TYPE(SS)> struct CHelper<true FOCP_FAKE_TYPE_ARG(SS)>
	{
		enum {value=false};
	};
public:
	enum {value=CHelper<CIsRefer<T>::value>::value};
};

/****************************************************
 *判断是否有变量修饰
*****************************************************/
template<typename T> struct CIsVolatile
{
private:
	template<bool FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper
	{
	private:
		template<bool FOCP_FAKE_DEFAULT_TYPE(SS)> struct CHelperA
		{
		private:
			static uint8 Test(const volatile void*);
			static uint16 Test(const void*);
		public:
			enum {value=sizeof(Test((T*)0))==sizeof(uint8)};
		};
		template<FOCP_FAKE_TYPE(SS)> struct CHelperA<true FOCP_FAKE_TYPE_ARG(SS)>
		{
		private:
			struct XX{ T t; };
			static uint8 Test(const volatile void*);
			static uint16 Test(const void*);
			static XX& MakeT();
		public:
			enum {value=sizeof(Test(&MakeT().t))==sizeof(uint8)};
		};
	public:
		enum {value=CHelperA<CIsArray<T>::value>::value};
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<true FOCP_FAKE_TYPE_ARG(S)>
	{
		enum {value=false};
	};
public:
	enum {value=CHelper<CIsRefer<T>::value>::value};
};

/****************************************************
 *删除常量变量修饰
*****************************************************/
#ifndef FOCP_NO_TEMPLATE_PARTIAL_SPECIALIZATION
FOCP_DETAIL_BEGIN();
template<typename T> struct CConstVolatileImp {};
template<typename T> struct CConstVolatileImp<T*>
{
	enum {is_const=false,is_volatile=false};
	typedef T unqualified;
};
template<typename T> struct CConstVolatileImp<const T*>
{
	enum {is_const=true,is_volatile=false};
	typedef T unqualified;
};
template<typename T> struct CConstVolatileImp<volatile T*>
{
	enum {is_const=false,is_volatile=true};
	typedef T unqualified;
};

template <typename T> struct CConstVolatileImp<const volatile T*>
{
	enum {is_const=true,is_volatile=true};
	typedef T unqualified;
};
FOCP_DETAIL_END();
#endif

/****************************************************
 *删除常量修饰
*****************************************************/
#ifndef FOCP_NO_TEMPLATE_PARTIAL_SPECIALIZATION
FOCP_DETAIL_BEGIN();
template<typename T, bool is_vol> struct CRemoveConstHelper{typedef T type;};
template<typename T> struct CRemoveConstHelper<T,true> {typedef T volatile type;};
template<typename T> struct CRemoveConstImp
{
	typedef typename CRemoveConstHelper<typename CConstVolatileImp<T*>::unqualified, CIsVolatile<T>::value>::type type;
};
FOCP_DETAIL_END();
template<typename T> struct CRemoveConst { typedef typename FOCP_DETAIL_NAME::CRemoveConstImp<T>::type type; };
template<typename T> struct CRemoveConst<T&> {typedef T& type; };
template<typename T,uint32 N> struct CRemoveConst<T const[N]> {typedef T type[N]; };
template<typename T,uint32 N> struct CRemoveConst<T const volatile [N]> {typedef T volatile type[N]; };
#elif defined(_MSC_VER)
FOCP_DETAIL_BEGIN();
template<bool IsPointer,bool IsArray,bool IsConst,bool IsVolatile> struct CRemoveConstHelper
{
	template<typename T,typename ID> struct inner {typedef T type;};
	template<typename T> struct transform{typedef T type;};
};
template<> struct CRemoveConstHelper<false,false,true,false>//Const
{
	template<typename T,typename ID> struct inner
	{
	private:
		template<typename U> static Trait::CRegisterType<U,ID> test(U const&(*)());
		static Trait::CRegisterType<T,ID> test(...);
		enum {value=sizeof(test((T(*)())0))};
	public:
		typedef typename Trait::CBaseType<ID>::id2type::type type;
	};
	template<typename T> struct transform{typedef T& type;};
};
template<> struct CRemoveConstHelper<false,false,true,true>//CV
{
	template<typename T,typename ID> struct inner
	{
	private:
		template<typename U> static Trait::CRegisterType<U,ID> test(U const volatile&(*)());
		static Trait::CRegisterType<T,ID> test(...);
		enum {value=sizeof(test((T(*)())0))};
	public:
		typedef typename Trait::CBaseType<ID>::id2type::type type;
	};
	template<typename T> struct transform{typedef T& type;};
};
template<> struct CRemoveConstHelper<true,false,true,false>//Const Pointer
{
	template<typename T,typename ID> struct inner
	{
	private:
		template<typename U> static Trait::CRegisterType<U,ID> test(void(*)(U const[]));
		static Trait::CRegisterType<T,ID> test(...);
		enum {value=sizeof(test((void(*)(T))0))};
	public:
		typedef typename Trait::CBaseType<ID>::id2type::type type;
	};
	template<typename T> struct transform{typedef T type[];};
};
template<> struct CRemoveConstHelper<true,false,true,true>//CV Pointer
{
	template<typename T,typename ID> struct inner
	{
	private:
		template<typename U> static Trait::CRegisterType<U,ID> test(void(*)(U const volatile[]));
		static Trait::CRegisterType<T,ID> test(...);
		enum {value=sizeof(test((void(*)(T))0))};
	public:
		typedef typename Trait::CBaseType<ID>::id2type::type type;
	};
	template<typename T> struct transform{typedef T type[];};
};
template<> struct CRemoveConstHelper<false,true,true,false>//Const Array
{
	template<typename T,typename ID> struct inner
	{
	private:
		enum {count=(sizeof(T)/sizeof((*(T*)0)[0]))};
		template<typename U> static Trait::CRegisterType<U[count],ID> test(void(*)(U const[]));
		static Trait::CRegisterType<T,ID> test(...);
		enum {value=sizeof(test((void(*)(T))0))};
	public:
		typedef typename Trait::CBaseType<ID>::id2type::type type;
	};
	template<typename T> struct transform{typedef T type;};
};
template<> struct CRemoveConstHelper<false,true,true,true>//CV Array
{
	template<typename T,typename ID> struct inner
	{
	private:
		enum {count=(sizeof(T)/sizeof((*(T*)0)[0]))};
		template<typename U> static Trait::CRegisterType<U[count],ID> test(void(*)(U const volatile[]));
		static Trait::CRegisterType<T,ID> test(...);
		enum {value=sizeof(test((void(*)(T))0))};
	public:
		typedef typename Trait::CBaseType<ID>::id2type::type type;
	};
	template<typename T> struct transform{typedef T type;};
};
FOCP_DETAIL_END();
template<typename T> struct CRemoveConst
{
private:
	typedef FOCP_DETAIL_NAME::CRemoveConstHelper<CIsPointer<T>::value, CIsArray<T>::value, CIsConst<T>::value, CIsVolatile<T>::value> CHelper;
public:
	typedef typename CHelper::inner< CHelper::transform<T>::type, CRemoveConst<T> >::type type;
};
#endif

template<typename T> struct CTryRemoveConst
{
private:
	template<bool FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper{typedef T type;};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<true FOCP_FAKE_TYPE_ARG(S)>{typedef typename CRemoveConst<T>::type type;};
public:
	typedef typename CHelper<CIsConst<T>::value>::type type;
};

/****************************************************
 *删除变量修饰
*****************************************************/
#ifndef FOCP_NO_TEMPLATE_PARTIAL_SPECIALIZATION
FOCP_DETAIL_BEGIN();
template<typename T, bool is_const> struct CRemoveVolatileHelper{typedef T type;};
template<typename T> struct CRemoveVolatileHelper<T,true> {typedef T const type;};
template<typename T> struct CRemoveVolatileImp
{
	typedef typename CRemoveVolatileHelper<typename CConstVolatileImp<T*>::unqualified, CIsConst<T>::value>::type type;
};
FOCP_DETAIL_END();
template<typename T> struct CRemoveVolatile { typedef typename FOCP_DETAIL_NAME::CRemoveVolatileImp<T>::type type; };
template<typename T> struct CRemoveVolatile<T&> {typedef T& type; };
template<typename T,uint32 N> struct CRemoveVolatile<T volatile[N]> {typedef T type[N]; };
template<typename T,uint32 N> struct CRemoveVolatile<T const volatile [N]> {typedef T const type[N]; };
#elif defined(_MSC_VER)
FOCP_DETAIL_BEGIN();
template<bool IsPointer,bool IsArray,bool IsConst,bool IsVolatile> struct CRemoveVolatileHelper
{
	template<typename T,typename ID> struct inner {typedef T type;};
	template<typename T> struct transform{typedef T type;};
};
template<> struct CRemoveVolatileHelper<false,false,false,true>//Volatile
{
	template<typename T,typename ID> struct inner
	{
	private:
		template<typename U> static Trait::CRegisterType<U,ID> test(U volatile&(*)());
		static Trait::CRegisterType<T,ID> test(...);
		enum {value=sizeof(test((T(*)())0))};
	public:
		typedef typename Trait::CBaseType<ID>::id2type::type type;
	};
	template<typename T> struct transform{typedef T& type;};
};
template<> struct CRemoveVolatileHelper<false,false,true,true>//CV
{
	template<typename T,typename ID> struct inner
	{
	private:
		template<typename U> static Trait::CRegisterType<U,ID> test(U const volatile&(*)());
		static Trait::CRegisterType<T,ID> test(...);
		enum {value=sizeof(test((T(*)())0))};
	public:
		typedef typename Trait::CBaseType<ID>::id2type::type type;
	};
	template<typename T> struct transform{typedef T& type;};
};
template<> struct CRemoveVolatileHelper<true,false,false,true>//Volatile Pointer
{
	template<typename T,typename ID> struct inner
	{
	private:
		template<typename U> static Trait::CRegisterType<U,ID> test(void(*)(U volatile[]));
		static Trait::CRegisterType<T,ID> test(...);
		enum {value=sizeof(test((void(*)(T))0))};
	public:
		typedef typename Trait::CBaseType<ID>::id2type::type type;
	};
	template<typename T> struct transform{typedef T type[];};
};
template<> struct CRemoveVolatileHelper<true,false,true,true>//CV Pointer
{
	template<typename T,typename ID> struct inner
	{
	private:
		template<typename U> static Trait::CRegisterType<U,ID> test(void(*)(U const volatile[]));
		static Trait::CRegisterType<T,ID> test(...);
		enum {value=sizeof(test((void(*)(T))0))};
	public:
		typedef typename Trait::CBaseType<ID>::id2type::type type;
	};
	template<typename T> struct transform{typedef T type[];};
};
template<> struct CRemoveVolatileHelper<false,true,false,true>//Volatile Array
{
	template<typename T,typename ID> struct inner
	{
	private:
		enum {count=(sizeof(T)/sizeof((*(T*)0)[0]))};
		template<typename U> static Trait::CRegisterType<U[count],ID> test(void(*)(U volatile[]));
		static Trait::CRegisterType<T,ID> test(...);
		enum {value=sizeof(test((void(*)(T))0))};
	public:
		typedef typename Trait::CBaseType<ID>::id2type::type type;
	};
	template<typename T> struct transform{typedef T type;};
};
template<> struct CRemoveVolatileHelper<false,true,true,true>//CV Array
{
	template<typename T,typename ID> struct inner
	{
	private:
		enum {count=(sizeof(T)/sizeof((*(T*)0)[0]))};
		template<typename U> static Trait::CRegisterType<U[count],ID> test(void(*)(U const volatile[]));
		static Trait::CRegisterType<T,ID> test(...);
		enum {value=sizeof(test((void(*)(T))0))};
	public:
		typedef typename Trait::CBaseType<ID>::id2type::type type;
	};
	template<typename T> struct transform{typedef T type;};
};
FOCP_DETAIL_END();
template<typename T> struct CRemoveVolatile
{
private:
	typedef FOCP_DETAIL_NAME::CRemoveVolatileHelper<CIsPointer<T>::value, CIsArray<T>::value, CIsConst<T>::value, CIsVolatile<T>::value> CHelper;
public:
	typedef typename CHelper::inner< CHelper::transform<T>::type, CRemoveVolatile<T> >::type type;
};
#endif

template<typename T> struct CTryRemoveVolatile
{
private:
	template<bool FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper{typedef T type;};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<true FOCP_FAKE_TYPE_ARG(S)>{typedef typename CRemoveVolatile<T>::type type;};
public:
	typedef typename CHelper<CIsVolatile<T>::value>::type type;
};

/****************************************************
 *判断返回值类型是否可写
*****************************************************/
template<typename T> struct CIsWritable
{
	enum {value=(CIsRefer<T>::value && !CIsConst<typename CRemoveRefer<T>::type>::value)};
};

/****************************************************
 *行断是否为整型
*****************************************************/
template<typename T> struct CIsIntegerType
{
private:
	typedef typename CRemoveRefer<T>::type t;
	template<typename t1, typename t2> struct CIsSameType2
	{
		enum
		{
			value=
				CIsSameType<t1,t2>::value||
			CIsSameType<t1,t2 const>::value||
			CIsSameType<t1,t2 volatile>::value||
			CIsSameType<t1,t2 const volatile>::value
		};
	};

public:
	enum
	{
		value=
			CIsSameType2<t,bool>::value||
			CIsSameType2<t,int8>::value||
			CIsSameType2<t,int16>::value||
			CIsSameType2<t,int32>::value||
			CIsSameType2<t,int64>::value||
			CIsSameType2<t,uint8>::value||
			CIsSameType2<t,uint16>::value||
			CIsSameType2<t,uint32>::value||
			CIsSameType2<t,uint64>::value||
			CIsSameType2<t,char>::value||
			CIsSameType2<t,short>::value||
			CIsSameType2<t,short int>::value||
			CIsSameType2<t,int>::value||
			CIsSameType2<t,long>::value||
			CIsSameType2<t,long int>::value||
			CIsSameType2<t,signed char>::value||
			CIsSameType2<t,signed short>::value||
			CIsSameType2<t,signed short int>::value||
			CIsSameType2<t,signed int>::value||
			CIsSameType2<t,signed long>::value||
			CIsSameType2<t,signed long int>::value||
			CIsSameType2<t,unsigned char>::value||
			CIsSameType2<t,unsigned short>::value||
			CIsSameType2<t,unsigned short int>::value||
			CIsSameType2<t,unsigned int>::value||
			CIsSameType2<t,unsigned long>::value||
			CIsSameType2<t,unsigned long int>::value
	};
};

/****************************************************
 *行断是否为浮点类型
*****************************************************/
template<typename T> struct CIsFloatType
{
	enum {	value=CIsSameType<T,double>::value||CIsSameType<T,float>::value };
};

/****************************************************
 *行断是否为数字类型
*****************************************************/
template<typename T> struct CIsNumberType
{
	enum { value=CIsIntegerType<T>::value|| CIsFloatType<T>::value };
};

/****************************************************
 *判断是否可以整数化
*****************************************************/
template<typename T> struct CAsIntegerType
{
private:
	template<bool FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper
	{
		enum {value=true};
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<false FOCP_FAKE_TYPE_ARG(S)>
	{
	private:
		typedef typename CRemoveRefer<T>::type t;

	public:
		enum
		{
			value=
				CExplicitConvertible<t,bool>::value||
				CExplicitConvertible<t,int8>::value||
				CExplicitConvertible<t,int16>::value||
				CExplicitConvertible<t,int32>::value||
				CExplicitConvertible<t,int64>::value||
				CExplicitConvertible<t,uint8>::value||
				CExplicitConvertible<t,uint16>::value||
				CExplicitConvertible<t,uint32>::value||
				CExplicitConvertible<t,uint64>::value||
				CExplicitConvertible<t,char>::value||
				CExplicitConvertible<t,short>::value||
				CExplicitConvertible<t,short int>::value||
				CExplicitConvertible<t,int>::value||
				CExplicitConvertible<t,long>::value||
				CExplicitConvertible<t,long int>::value||
				CExplicitConvertible<t,signed char>::value||
				CExplicitConvertible<t,signed short>::value||
				CExplicitConvertible<t,signed short int>::value||
				CExplicitConvertible<t,signed int>::value||
				CExplicitConvertible<t,signed long>::value||
				CExplicitConvertible<t,signed long int>::value||
				CExplicitConvertible<t,unsigned char>::value||
				CExplicitConvertible<t,unsigned short>::value||
				CExplicitConvertible<t,unsigned short int>::value||
				CExplicitConvertible<t,unsigned int>::value||
				CExplicitConvertible<t,unsigned long>::value||
				CExplicitConvertible<t,unsigned long int>::value
		};
	};
public:
	enum {value=CHelper<CIsIntegerType<T>::value>::value};
};

/****************************************************
 *判断是否可以浮点化
*****************************************************/
template<typename T> struct CAsFloatType
{
private:
	template<bool FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper
	{
		enum {value=true};
	};
	template<FOCP_FAKE_TYPE(S)> struct CHelper<false FOCP_FAKE_TYPE_ARG(S)>
	{
	private:
		typedef typename CRemoveRefer<T>::type t;
	public:
		enum {	value=CExplicitConvertible<t,double>::value || CExplicitConvertible<t,float>::value};
	};
public:
	enum {value=CHelper<CIsFloatType<T>::value>::value};
};

/****************************************************
 *判断是否可以数字化
*****************************************************/
template<typename T> struct CAsNumberType
{
	enum { value=CAsIntegerType<T>::value|| CAsFloatType<T>::value };
};

/****************************************************
 *数字类型特征
*****************************************************/
template<typename T> struct CNumberAttr
{
private:
	template<uint32 SIZE, bool FLOAT, bool SIGNED FOCP_FAKE_DEFAULT_TYPE(S)> struct CPriorityHelper{};
	template<FOCP_FAKE_TYPE(S)> struct CPriorityHelper<1,false,true FOCP_FAKE_TYPE_ARG(S)>{enum{value=1};};
	template<FOCP_FAKE_TYPE(S)> struct CPriorityHelper<1,false,false FOCP_FAKE_TYPE_ARG(S)>{enum{value=2};};
	template<FOCP_FAKE_TYPE(S)> struct CPriorityHelper<2,false,true FOCP_FAKE_TYPE_ARG(S)>{enum{value=3};};
	template<FOCP_FAKE_TYPE(S)> struct CPriorityHelper<2,false,false FOCP_FAKE_TYPE_ARG(S)>{enum{value=4};};
	template<FOCP_FAKE_TYPE(S)> struct CPriorityHelper<4,false,true FOCP_FAKE_TYPE_ARG(S)>{enum{value=5};};
	template<FOCP_FAKE_TYPE(S)> struct CPriorityHelper<4,false,false FOCP_FAKE_TYPE_ARG(S)>{enum{value=6};};
	template<FOCP_FAKE_TYPE(S)> struct CPriorityHelper<4,true,true FOCP_FAKE_TYPE_ARG(S)>{enum{value=7};};
	template<FOCP_FAKE_TYPE(S)> struct CPriorityHelper<8,true,true FOCP_FAKE_TYPE_ARG(S)>{enum{value=8};};

public:
	enum
	{
		nSize = sizeof(T),
		bFloat = CIsFloatType<T>::value,
		bSigned = (0>((T)0-(T)1)),
		nPriority = CPriorityHelper<nSize,bFloat,bSigned>::value
	};
};
template<> struct CNumberAttr<bool>
{
	enum
	{
		nSize = sizeof(bool),
		bFloat = false,
		bSigned = false,
		nPriority = 0
	};
};

template<typename A, typename B> struct CHighPriorityType
{
private:
	template<bool FOCP_FAKE_DEFAULT_TYPE(S)> struct CHelper	{ typedef A type; };
	template<FOCP_FAKE_TYPE(S)> struct CHelper<false FOCP_FAKE_TYPE_ARG(S)>	{ typedef B type; };
	enum{cond = (CNumberAttr<A>::nPriority > CNumberAttr<B>::nPriority)};
public:
	typedef typename CHelper<cond>::type type;
};

template<typename T> struct CRemoveSignedAndUnsigned
{
	typedef typename FOCP_DETAIL_NAME::integer<CNumberAttr<T>::nSize*8>::int_t type;
};

template<typename T> struct CAddSigned
{
	typedef typename FOCP_DETAIL_NAME::integer<CNumberAttr<T>::nSize*8>::signed_int type;
};

template<typename T> struct CAddUnsigned
{
	typedef typename FOCP_DETAIL_NAME::integer<CNumberAttr<T>::nSize*8>::unsigned_int type;
};

FOCP_END();

#endif
