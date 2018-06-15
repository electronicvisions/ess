//_____________________________________________
// Company      :	Tu-Dresden
// Author       :	Stefan Scholze
// E-Mail   	:	scholze@iee.et.tu-dresden.de
//
// Date         :   Wed Jan 17 09:18:19 2007
// Last Change  :   mehrlich 03/27/2008
// Module Name  :   generic array
// Filename     :   generic_array.h
// Project Name	:   p_facets/s_systemsim
// Description	:
//
//_____________________________________________

#ifndef _GENERIC_ARRAY_H_
#define _GENERIC_ARRAY_H_
namespace ess
{

template <bool Cond, typename T1, typename T2>
struct select_type
{
	typedef T1 Type;
};

template <typename T1, typename T2>
struct select_type<false, T1, T2>
{
	typedef T2 Type;
};

/// This struct provides a const reference for access to the generic array.
template <typename T, size_t A, size_t B=1, size_t C=1, size_t D=1>
struct array_reference_const
{
	enum { single_element = (B==1 && C==1 && D==1) };
	typedef typename select_type<single_element, T const &,array_reference_const<T,B,C,D,1> >::Type const_subscript_t;

	const T* arr;
	array_reference_const(const T& a) : arr(&a)
	{}
	const_subscript_t operator[](int i) const
	{
		return subscript_t(arr[i*B*C*D]);
	}

	const T* operator()() const	{ return this->arr; }
	size_t size() const 		{ return A*B*C*D; }
};


/// This struct provides a reference for access to the generic array.
template <typename T, size_t A, size_t B=1, size_t C=1, size_t D=1>
struct array_reference
{
	enum { single_element = (B==1 && C==1 && D==1) };
	typedef typename select_type<single_element, T&,array_reference<T,B,C,D,1> >::Type subscript_t;
	typedef typename select_type<single_element, T const &,array_reference_const<T,B,C,D,1> >::Type const_subscript_t;

	T* arr;
	array_reference(T& a) : arr(&a)
	{}

	operator array_reference_const<T,A,B,C,D>()	const	{ return array_reference_const<T,A,B,C,D>(*arr); }

	subscript_t operator[](int i)
	{
		return subscript_t(arr[i*B*C*D]);
	}
	const_subscript_t operator[](int i) const
	{
		return const_subscript_t(arr[i*B*C*D]);
	}

	T* operator()()				{ return this->arr; }
	const T* operator()() const	{ return this->arr; }
	size_t size() const 		{ return A*B*C*D; }
};

/// This struct provides a generic array.
template <typename T, size_t A, size_t B=1, size_t C=1, size_t D=1>
struct generic_array
{
	enum { single_element = (B==1 && C==1 && D==1) };
	typedef typename select_type<single_element, T&,array_reference<T,B,C,D,1> >::Type subscript_t;
	typedef typename select_type<single_element, T const &,array_reference_const<T,B,C,D,1> >::Type const_subscript_t;

	T *arr;
	operator array_reference<T,A,B,C,D>()				{ return array_reference<T,A,B,C,D>(*arr); }
	operator array_reference_const<T,A,B,C,D>()	const	{ return array_reference_const<T,A,B,C,D>(*arr); }

	generic_array()
		: arr(new T[A*B*C*D])
	{}
	~generic_array()
	{
		delete[] this->arr;
	}

	generic_array(const generic_array& a)
		: arr(new T[A*B*C*D])
	{
		size_t cnt = A*B*C*D;
		const T* rpt = a.arr;
		T* wpt = this->arr;
		for(; cnt; --cnt, ++rpt, ++wpt)
			*wpt = *rpt;
	}

    // the next line produced the compiler warning: "no return statement in function returning non-void",
    // so I changed it (Fieres, 19.10.07)
    //	const generic_array operator=(const generic_array& a)
	generic_array& operator=(const generic_array& a)
	{
		size_t cnt = A*B*C*D;
		const T* rpt = a.arr;
		T* wpt = this->arr;
		for(; cnt; --cnt, ++rpt, ++wpt)
			*wpt = *rpt;
		// added this (Fieres 19.10.07)
                return *this;
	}

	subscript_t operator[](int i)
	{
		return subscript_t(arr[i*B*C*D]);
	}
	const_subscript_t operator[](int i) const
	{
		return const_subscript_t(arr[i*B*C*D]);
	}
	T* operator()()				{ return this->arr; }
	const T* operator()() const	{ return this->arr; }
	size_t size() const 		{ return A*B*C*D; }
};

} // end namespace ess

#endif//_GENERIC_ARRAY_H_
