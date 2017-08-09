/// @ref core
/// @file glm/detail/func_vector_relational.inl

#include "compute_vector_relational.hpp"

namespace glm
{
	template<length_t L, typename T, precision P>
	GLM_FUNC_QUALIFIER vec<L, bool, P> lessThan(vec<L, T, P> const& x, vec<L, T, P> const& y)
	{
		assert(x.length() == y.length());

		vec<L, bool, P> Result;
		for(length_t i = 0; i < x.length(); ++i)
			Result[i] = x[i] < y[i];

		return Result;
	}

	template<length_t L, typename T, precision P>
	GLM_FUNC_QUALIFIER vec<L, bool, P> lessThanEqual(vec<L, T, P> const& x, vec<L, T, P> const& y)
	{
		assert(x.length() == y.length());

		vec<L, bool, P> Result;
		for(length_t i = 0; i < x.length(); ++i)
			Result[i] = x[i] <= y[i];
		return Result;
	}

	template<length_t L, typename T, precision P>
	GLM_FUNC_QUALIFIER vec<L, bool, P> greaterThan(vec<L, T, P> const& x, vec<L, T, P> const& y)
	{
		assert(x.length() == y.length());

		vec<L, bool, P> Result;
		for(length_t i = 0; i < x.length(); ++i)
			Result[i] = x[i] > y[i];
		return Result;
	}

	template<length_t L, typename T, precision P>
	GLM_FUNC_QUALIFIER vec<L, bool, P> greaterThanEqual(vec<L, T, P> const& x, vec<L, T, P> const& y)
	{
		assert(x.length() == y.length());

		vec<L, bool, P> Result;
		for(length_t i = 0; i < x.length(); ++i)
			Result[i] = x[i] >= y[i];
		return Result;
	}

	template<length_t L, typename T, precision P>
	GLM_FUNC_QUALIFIER vec<L, bool, P> equal(vec<L, T, P> const& x, vec<L, T, P> const& y)
	{
		assert(x.length() == y.length());

		vec<L, bool, P> Result;
		for(length_t i = 0; i < x.length(); ++i)
			Result[i] = detail::compute_equal<T>::call(x[i], y[i]);
		return Result;
	}

	template<length_t L, typename T, precision P>
	GLM_FUNC_QUALIFIER vec<L, bool, P> notEqual(vec<L, T, P> const& x, vec<L, T, P> const& y)
	{
		assert(x.length() == y.length());

		vec<L, bool, P> Result;
		for(length_t i = 0; i < x.length(); ++i)
			Result[i] = !detail::compute_equal<T>::call(x[i], y[i]);
		return Result;
	}

	template<length_t L, precision P>
	GLM_FUNC_QUALIFIER bool any(vec<L, bool, P> const& v)
	{
		bool Result = false;
		for(length_t i = 0; i < v.length(); ++i)
			Result = Result || v[i];
		return Result;
	}

	template<length_t L, precision P>
	GLM_FUNC_QUALIFIER bool all(vec<L, bool, P> const& v)
	{
		bool Result = true;
		for(length_t i = 0; i < v.length(); ++i)
			Result = Result && v[i];
		return Result;
	}

	template<length_t L, precision P>
	GLM_FUNC_QUALIFIER vec<L, bool, P> not_(vec<L, bool, P> const& v)
	{
		vec<L, bool, P> Result;
		for(length_t i = 0; i < v.length(); ++i)
			Result[i] = !v[i];
		return Result;
	}
}//namespace glm

#if GLM_ARCH != GLM_ARCH_PURE && GLM_HAS_UNRESTRICTED_UNIONS
#	include "func_vector_relational_simd.inl"
#endif
