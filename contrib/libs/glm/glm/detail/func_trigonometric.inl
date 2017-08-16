/// @ref core
/// @file glm/detail/func_trigonometric.inl

#include "_vectorize.hpp"
#include <cmath>
#include <limits>

namespace glm
{
	// radians
	template<typename genType>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR genType radians(genType degrees)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<genType>::is_iec559, "'radians' only accept floating-point input");

		return degrees * static_cast<genType>(0.01745329251994329576923690768489);
	}

	template<length_t L, typename T, precision P>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR vec<L, T, P> radians(vec<L, T, P> const& v)
	{
		return detail::functor1<L, T, T, P>::call(radians, v);
	}
	
	// degrees
	template<typename genType>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR genType degrees(genType radians)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<genType>::is_iec559, "'degrees' only accept floating-point input");

		return radians * static_cast<genType>(57.295779513082320876798154814105);
	}

	template<length_t L, typename T, precision P>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR vec<L, T, P> degrees(vec<L, T, P> const& v)
	{
		return detail::functor1<L, T, T, P>::call(degrees, v);
	}

	// sin
	using ::std::sin;

	template<length_t L, typename T, precision P>
	GLM_FUNC_QUALIFIER vec<L, T, P> sin(vec<L, T, P> const& v)
	{
		return detail::functor1<L, T, T, P>::call(sin, v);
	}

	// cos
	using std::cos;

	template<length_t L, typename T, precision P>
	GLM_FUNC_QUALIFIER vec<L, T, P> cos(vec<L, T, P> const& v)
	{
		return detail::functor1<L, T, T, P>::call(cos, v);
	}

	// tan
	using std::tan;

	template<length_t L, typename T, precision P>
	GLM_FUNC_QUALIFIER vec<L, T, P> tan(vec<L, T, P> const& v)
	{
		return detail::functor1<L, T, T, P>::call(tan, v);
	}

	// asin
	using std::asin;

	template<length_t L, typename T, precision P>
	GLM_FUNC_QUALIFIER vec<L, T, P> asin(vec<L, T, P> const& v)
	{
		return detail::functor1<L, T, T, P>::call(asin, v);
	}

	// acos
	using std::acos;

	template<length_t L, typename T, precision P>
	GLM_FUNC_QUALIFIER vec<L, T, P> acos(vec<L, T, P> const& v)
	{
		return detail::functor1<L, T, T, P>::call(acos, v);
	}

	// atan
	template<typename genType>
	GLM_FUNC_QUALIFIER genType atan(genType y, genType x)
	{
		GLM_STATIC_ASSERT(std::numeric_limits<genType>::is_iec559, "'atan' only accept floating-point input");

		return ::std::atan2(y, x);
	}

	template<length_t L, typename T, precision P>
	GLM_FUNC_QUALIFIER vec<L, T, P> atan(vec<L, T, P> const& a, vec<L, T, P> const& b)
	{
		return detail::functor2<L, T, P>::call(::std::atan2, a, b);
	}

	using std::atan;

	template<length_t L, typename T, precision P>
	GLM_FUNC_QUALIFIER vec<L, T, P> atan(vec<L, T, P> const& v)
	{
		return detail::functor1<L, T, T, P>::call(atan, v);
	}

	// sinh
	using std::sinh;

	template<length_t L, typename T, precision P>
	GLM_FUNC_QUALIFIER vec<L, T, P> sinh(vec<L, T, P> const& v)
	{
		return detail::functor1<L, T, T, P>::call(sinh, v);
	}

	// cosh
	using std::cosh;

	template<length_t L, typename T, precision P>
	GLM_FUNC_QUALIFIER vec<L, T, P> cosh(vec<L, T, P> const& v)
	{
		return detail::functor1<L, T, T, P>::call(cosh, v);
	}

	// tanh
	using std::tanh;

	template<length_t L, typename T, precision P>
	GLM_FUNC_QUALIFIER vec<L, T, P> tanh(vec<L, T, P> const& v)
	{
		return detail::functor1<L, T, T, P>::call(tanh, v);
	}

	// asinh
#	if GLM_HAS_CXX11_STL
		using std::asinh;
#	else
		template<typename genType>
		GLM_FUNC_QUALIFIER genType asinh(genType x)
		{
			GLM_STATIC_ASSERT(std::numeric_limits<genType>::is_iec559, "'asinh' only accept floating-point input");

			return (x < static_cast<genType>(0) ? static_cast<genType>(-1) : (x > static_cast<genType>(0) ? static_cast<genType>(1) : static_cast<genType>(0))) * log(std::abs(x) + sqrt(static_cast<genType>(1) + x * x));
		}
#	endif

	template<length_t L, typename T, precision P>
	GLM_FUNC_QUALIFIER vec<L, T, P> asinh(vec<L, T, P> const& v)
	{
		return detail::functor1<L, T, T, P>::call(asinh, v);
	}

	// acosh
#	if GLM_HAS_CXX11_STL
		using std::acosh;
#	else
		template<typename genType> 
		GLM_FUNC_QUALIFIER genType acosh(genType x)
		{
			GLM_STATIC_ASSERT(std::numeric_limits<genType>::is_iec559, "'acosh' only accept floating-point input");

			if(x < static_cast<genType>(1))
				return static_cast<genType>(0);
			return log(x + sqrt(x * x - static_cast<genType>(1)));
		}
#	endif

	template<length_t L, typename T, precision P>
	GLM_FUNC_QUALIFIER vec<L, T, P> acosh(vec<L, T, P> const& v)
	{
		return detail::functor1<L, T, T, P>::call(acosh, v);
	}

	// atanh
#	if GLM_HAS_CXX11_STL
		using std::atanh;
#	else
		template<typename genType>
		GLM_FUNC_QUALIFIER genType atanh(genType x)
		{
			GLM_STATIC_ASSERT(std::numeric_limits<genType>::is_iec559, "'atanh' only accept floating-point input");
		
			if(std::abs(x) >= static_cast<genType>(1))
				return 0;
			return static_cast<genType>(0.5) * log((static_cast<genType>(1) + x) / (static_cast<genType>(1) - x));
		}
#	endif

	template<length_t L, typename T, precision P>
	GLM_FUNC_QUALIFIER vec<L, T, P> atanh(vec<L, T, P> const& v)
	{
		return detail::functor1<L, T, T, P>::call(atanh, v);
	}
}//namespace glm

#if GLM_ARCH != GLM_ARCH_PURE && GLM_HAS_UNRESTRICTED_UNIONS
#	include "func_trigonometric_simd.inl"
#endif

