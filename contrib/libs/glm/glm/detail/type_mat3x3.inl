#include "../matrix.hpp"
#include "../common.hpp"

namespace glm
{
	// -- Constructors --

#	if GLM_CONFIG_CTOR_INIT == GLM_ENABLE
		template<typename T, qualifier Q>
		GLM_DEFAULTED_DEFAULT_CTOR_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q>::mat()
			: value{col_type(1, 0, 0), col_type(0, 1, 0), col_type(0, 0, 1)}
		{}
#	endif

	template<typename T, qualifier Q>
	template<qualifier P>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q>::mat(mat<3, 3, T, P> const& m)
		: value{col_type(m[0]), col_type(m[1]), col_type(m[2])}
	{}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q>::mat(T s)
		: value{col_type(s, 0, 0), col_type(0, s, 0), col_type(0, 0, s)}
	{}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q>::mat
	(
		T x0, T y0, T z0,
		T x1, T y1, T z1,
		T x2, T y2, T z2
	)
		: value{col_type(x0, y0, z0), col_type(x1, y1, z1), col_type(x2, y2, z2)}
	{}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q>::mat(col_type const& v0, col_type const& v1, col_type const& v2)
		: value{col_type(v0), col_type(v1), col_type(v2)}
	{}

	// -- Conversion constructors --

	template<typename T, qualifier Q>
	template<
		typename X1, typename Y1, typename Z1,
		typename X2, typename Y2, typename Z2,
		typename X3, typename Y3, typename Z3>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q>::mat
	(
		X1 x1, Y1 y1, Z1 z1,
		X2 x2, Y2 y2, Z2 z2,
		X3 x3, Y3 y3, Z3 z3
	)
		: value{col_type(x1, y1, z1), col_type(x2, y2, z2), col_type(x3, y3, z3)}
	{}

	template<typename T, qualifier Q>
	template<typename V1, typename V2, typename V3>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q>::mat(vec<3, V1, Q> const& v1, vec<3, V2, Q> const& v2, vec<3, V3, Q> const& v3)
		: value{col_type(v1), col_type(v2), col_type(v3)}
	{}

	// -- Matrix conversions --

	template<typename T, qualifier Q>
	template<typename U, qualifier P>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q>::mat(mat<3, 3, U, P> const& m)
		: value{col_type(m[0]), col_type(m[1]), col_type(m[2])}
	{}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q>::mat(mat<2, 2, T, Q> const& m)
		: value{col_type(m[0], 0), col_type(m[1], 0), col_type(0, 0, 1)}
	{}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q>::mat(mat<4, 4, T, Q> const& m)
		: value{col_type(m[0]), col_type(m[1]), col_type(m[2])}
	{}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q>::mat(mat<2, 3, T, Q> const& m)
		: value{col_type(m[0]), col_type(m[1]), col_type(0, 0, 1)}
	{}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q>::mat(mat<3, 2, T, Q> const& m)
		: value{col_type(m[0], 0), col_type(m[1], 0), col_type(m[2], 1)}
	{}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q>::mat(mat<2, 4, T, Q> const& m)
		: value{col_type(m[0]), col_type(m[1]), col_type(0, 0, 1)}
	{}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q>::mat(mat<4, 2, T, Q> const& m)
		: value{col_type(m[0], 0), col_type(m[1], 0), col_type(m[2], 1)}
	{}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q>::mat(mat<3, 4, T, Q> const& m)
		: value{col_type(m[0]), col_type(m[1]), col_type(m[2])}
	{}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q>::mat(mat<4, 3, T, Q> const& m)
		: value{col_type(m[0]), col_type(m[1]), col_type(m[2])}
	{}

	// -- Accesses --

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR typename mat<3, 3, T, Q>::col_type & mat<3, 3, T, Q>::operator[](typename mat<3, 3, T, Q>::length_type i) noexcept
	{
		GLM_ASSERT_LENGTH(i, this->length());
		return this->value[i];
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR typename mat<3, 3, T, Q>::col_type const& mat<3, 3, T, Q>::operator[](typename mat<3, 3, T, Q>::length_type i) const noexcept
	{
		GLM_ASSERT_LENGTH(i, this->length());
		return this->value[i];
	}

	// -- Unary updatable operators --

	template<typename T, qualifier Q>
	template<typename U>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q> & mat<3, 3, T, Q>::operator=(mat<3, 3, U, Q> const& m)
	{
		this->value[0] = m[0];
		this->value[1] = m[1];
		this->value[2] = m[2];
		return *this;
	}

	template<typename T, qualifier Q>
	template<typename U>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q> & mat<3, 3, T, Q>::operator+=(U s)
	{
		this->value[0] += s;
		this->value[1] += s;
		this->value[2] += s;
		return *this;
	}

	template<typename T, qualifier Q>
	template<typename U>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q> & mat<3, 3, T, Q>::operator+=(mat<3, 3, U, Q> const& m)
	{
		this->value[0] += m[0];
		this->value[1] += m[1];
		this->value[2] += m[2];
		return *this;
	}

	template<typename T, qualifier Q>
	template<typename U>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q> & mat<3, 3, T, Q>::operator-=(U s)
	{
		this->value[0] -= s;
		this->value[1] -= s;
		this->value[2] -= s;
		return *this;
	}

	template<typename T, qualifier Q>
	template<typename U>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q> & mat<3, 3, T, Q>::operator-=(mat<3, 3, U, Q> const& m)
	{
		this->value[0] -= m[0];
		this->value[1] -= m[1];
		this->value[2] -= m[2];
		return *this;
	}

	template<typename T, qualifier Q>
	template<typename U>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q> & mat<3, 3, T, Q>::operator*=(U s)
	{
		col_type sv(s);
		this->value[0] *= sv;
		this->value[1] *= sv;
		this->value[2] *= sv;
		return *this;
	}

	template<typename T, qualifier Q>
	template<typename U>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q> & mat<3, 3, T, Q>::operator*=(mat<3, 3, U, Q> const& m)
	{
		return (*this = *this * m);
	}

	template<typename T, qualifier Q>
	template<typename U>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q> & mat<3, 3, T, Q>::operator/=(U s)
	{
		this->value[0] /= s;
		this->value[1] /= s;
		this->value[2] /= s;
		return *this;
	}

	template<typename T, qualifier Q>
	template<typename U>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q> & mat<3, 3, T, Q>::operator/=(mat<3, 3, U, Q> const& m)
	{
		return *this *= inverse(m);
	}

	// -- Increment and decrement operators --

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q> & mat<3, 3, T, Q>::operator++()
	{
		++this->value[0];
		++this->value[1];
		++this->value[2];
		return *this;
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q> & mat<3, 3, T, Q>::operator--()
	{
		--this->value[0];
		--this->value[1];
		--this->value[2];
		return *this;
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q> mat<3, 3, T, Q>::operator++(int)
	{
		mat<3, 3, T, Q> Result(*this);
		++*this;
		return Result;
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q> mat<3, 3, T, Q>::operator--(int)
	{
		mat<3, 3, T, Q> Result(*this);
		--*this;
		return Result;
	}

	// -- Unary arithmetic operators --

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q> operator+(mat<3, 3, T, Q> const& m)
	{
		return m;
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q> operator-(mat<3, 3, T, Q> const& m)
	{
		return mat<3, 3, T, Q>(
			-m[0],
			-m[1],
			-m[2]);
	}

	// -- Binary arithmetic operators --

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q> operator+(mat<3, 3, T, Q> const& m, T scalar)
	{
		return mat<3, 3, T, Q>(
			m[0] + scalar,
			m[1] + scalar,
			m[2] + scalar);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q> operator+(T scalar, mat<3, 3, T, Q> const& m)
	{
		return mat<3, 3, T, Q>(
			m[0] + scalar,
			m[1] + scalar,
			m[2] + scalar);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q> operator+(mat<3, 3, T, Q> const& m1, mat<3, 3, T, Q> const& m2)
	{
		return mat<3, 3, T, Q>(
			m1[0] + m2[0],
			m1[1] + m2[1],
			m1[2] + m2[2]);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q> operator-(mat<3, 3, T, Q> const& m, T scalar)
	{
		return mat<3, 3, T, Q>(
			m[0] - scalar,
			m[1] - scalar,
			m[2] - scalar);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q> operator-(T scalar, mat<3, 3, T, Q> const& m)
	{
		return mat<3, 3, T, Q>(
			scalar - m[0],
			scalar - m[1],
			scalar - m[2]);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q> operator-(mat<3, 3, T, Q> const& m1, mat<3, 3, T, Q> const& m2)
	{
		return mat<3, 3, T, Q>(
			m1[0] - m2[0],
			m1[1] - m2[1],
			m1[2] - m2[2]);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q> operator*(mat<3, 3, T, Q> const& m, T scalar)
	{
		return mat<3, 3, T, Q>(
			m[0] * scalar,
			m[1] * scalar,
			m[2] * scalar);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q> operator*(T scalar, mat<3, 3, T, Q> const& m)
	{
		return mat<3, 3, T, Q>(
			m[0] * scalar,
			m[1] * scalar,
			m[2] * scalar);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR typename mat<3, 3, T, Q>::col_type operator*(mat<3, 3, T, Q> const& m, typename mat<3, 3, T, Q>::row_type const& v)
	{
		return typename mat<3, 3, T, Q>::col_type(
			m[0] * splatX(v) + m[1] * splatY(v) + m[2] * splatZ(v));
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR typename mat<3, 3, T, Q>::row_type operator*(typename mat<3, 3, T, Q>::col_type const& v, mat<3, 3, T, Q> const& m)
	{
		return typename mat<3, 3, T, Q>::row_type(
			dot(m[0], v),
			dot(m[1], v),
			dot(m[2], v));
	}

	namespace detail
	{
		template<typename T, qualifier Q, bool is_aligned>
		struct mul3x3 {};

#if GLM_CONFIG_SIMD == GLM_ENABLE
		template<typename T, qualifier Q>
		struct mul3x3<T, Q, true>
		{
			GLM_FUNC_QUALIFIER static mat<3, 3, T, Q> call(mat<3, 3, T, Q> const& m1, mat<3, 3, T, Q> const& m2)
			{
				typename mat<4, 4, T, Q>::col_type const SrcA0 = xyzz(m1[0]);
				typename mat<4, 4, T, Q>::col_type const SrcA1 = xyzz(m1[1]);
				typename mat<4, 4, T, Q>::col_type const SrcA2 = xyzz(m1[2]);

				typename mat<4, 4, T, Q>::col_type const SrcB0 = xyzz(m2[0]);
				typename mat<4, 4, T, Q>::col_type const SrcB1 = xyzz(m2[1]);
				typename mat<4, 4, T, Q>::col_type const SrcB2 = xyzz(m2[2]);

				typename mat<3, 3, T, Q>::col_type const tmp0 = xyz(glm::fma(SrcA2, splatZ(SrcB0), glm::fma(SrcA1, splatY(SrcB0), SrcA0 * splatX(SrcB0))));
				typename mat<3, 3, T, Q>::col_type const tmp1 = xyz(glm::fma(SrcA2, splatZ(SrcB1), glm::fma(SrcA1, splatY(SrcB1), SrcA0 * splatX(SrcB1))));
				typename mat<3, 3, T, Q>::col_type const tmp2 = xyz(glm::fma(SrcA2, splatZ(SrcB2), glm::fma(SrcA1, splatY(SrcB2), SrcA0 * splatX(SrcB2))));

				return mat<3, 3, T, Q>(tmp0, tmp1, tmp2);
			}
		};
#endif
		template<typename T, qualifier Q>
		struct mul3x3<T, Q, false>
		{
			GLM_FUNC_QUALIFIER static mat<3, 3, T, Q> call(mat<3, 3, T, Q> const& m1, mat<3, 3, T, Q> const& m2)
			{
				typename mat<3, 3, T, Q>::col_type const& SrcA0 = m1[0];
				typename mat<3, 3, T, Q>::col_type const& SrcA1 = m1[1];
				typename mat<3, 3, T, Q>::col_type const& SrcA2 = m1[2];

				typename mat<3, 3, T, Q>::col_type const& SrcB0 = m2[0];
				typename mat<3, 3, T, Q>::col_type const& SrcB1 = m2[1];
				typename mat<3, 3, T, Q>::col_type const& SrcB2 = m2[2];

				// note: the following lines are decomposed to have consistent results between simd and non simd code (prevent rounding error because of operation order)
				//Result[0] = SrcA2 * SrcB0.z + SrcA1 * SrcB0.y + SrcA0 * SrcB0.x;
				//Result[1] = SrcA2 * SrcB1.z + SrcA1 * SrcB1.y + SrcA0 * SrcB1.x;
				//Result[2] = SrcA2 * SrcB2.z + SrcA1 * SrcB2.y + SrcA0 * SrcB2.x;

				typename mat<3, 3, T, Q>::col_type tmp0 = SrcA0 * SrcB0.x;
				tmp0 += SrcA1 * SrcB0.y;
				tmp0 += SrcA2 * SrcB0.z;
				typename mat<3, 3, T, Q>::col_type tmp1 = SrcA0 * SrcB1.x;
				tmp1 += SrcA1 * SrcB1.y;
				tmp1 += SrcA2 * SrcB1.z;
				typename mat<3, 3, T, Q>::col_type tmp2 = SrcA0 * SrcB2.x;
				tmp2 += SrcA1 * SrcB2.y;
				tmp2 += SrcA2 * SrcB2.z;

				return mat<3, 3, T, Q>(tmp0, tmp1, tmp2);
			}
		};
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q> operator*(mat<3, 3, T, Q> const& m1, mat<3, 3, T, Q> const& m2)
	{
		return detail::mul3x3<T, Q, detail::is_aligned<Q>::value>::call(m1, m2);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<2, 3, T, Q> operator*(mat<3, 3, T, Q> const& m1, mat<2, 3, T, Q> const& m2)
	{
		return mat<2, 3, T, Q>(
			m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1] + m1[2][0] * m2[0][2],
			m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1] + m1[2][1] * m2[0][2],
			m1[0][2] * m2[0][0] + m1[1][2] * m2[0][1] + m1[2][2] * m2[0][2],
			m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1] + m1[2][0] * m2[1][2],
			m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1] + m1[2][1] * m2[1][2],
			m1[0][2] * m2[1][0] + m1[1][2] * m2[1][1] + m1[2][2] * m2[1][2]);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<4, 3, T, Q> operator*(mat<3, 3, T, Q> const& m1, mat<4, 3, T, Q> const& m2)
	{
		return mat<4, 3, T, Q>(
			m1[0][0] * m2[0][0] + m1[1][0] * m2[0][1] + m1[2][0] * m2[0][2],
			m1[0][1] * m2[0][0] + m1[1][1] * m2[0][1] + m1[2][1] * m2[0][2],
			m1[0][2] * m2[0][0] + m1[1][2] * m2[0][1] + m1[2][2] * m2[0][2],
			m1[0][0] * m2[1][0] + m1[1][0] * m2[1][1] + m1[2][0] * m2[1][2],
			m1[0][1] * m2[1][0] + m1[1][1] * m2[1][1] + m1[2][1] * m2[1][2],
			m1[0][2] * m2[1][0] + m1[1][2] * m2[1][1] + m1[2][2] * m2[1][2],
			m1[0][0] * m2[2][0] + m1[1][0] * m2[2][1] + m1[2][0] * m2[2][2],
			m1[0][1] * m2[2][0] + m1[1][1] * m2[2][1] + m1[2][1] * m2[2][2],
			m1[0][2] * m2[2][0] + m1[1][2] * m2[2][1] + m1[2][2] * m2[2][2],
			m1[0][0] * m2[3][0] + m1[1][0] * m2[3][1] + m1[2][0] * m2[3][2],
			m1[0][1] * m2[3][0] + m1[1][1] * m2[3][1] + m1[2][1] * m2[3][2],
			m1[0][2] * m2[3][0] + m1[1][2] * m2[3][1] + m1[2][2] * m2[3][2]);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q> operator/(mat<3, 3, T, Q> const& m,	T scalar)
	{
		return mat<3, 3, T, Q>(
			m[0] / scalar,
			m[1] / scalar,
			m[2] / scalar);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q> operator/(T scalar, mat<3, 3, T, Q> const& m)
	{
		return mat<3, 3, T, Q>(
			scalar / m[0],
			scalar / m[1],
			scalar / m[2]);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR typename mat<3, 3, T, Q>::col_type operator/(mat<3, 3, T, Q> const& m, typename mat<3, 3, T, Q>::row_type const& v)
	{
		return  inverse(m) * v;
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR typename mat<3, 3, T, Q>::row_type operator/(typename mat<3, 3, T, Q>::col_type const& v, mat<3, 3, T, Q> const& m)
	{
		return v * inverse(m);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR mat<3, 3, T, Q> operator/(mat<3, 3, T, Q> const& m1, mat<3, 3, T, Q> const& m2)
	{
		mat<3, 3, T, Q> m1_copy(m1);
		return m1_copy /= m2;
	}

	// -- Boolean operators --

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR bool operator==(mat<3, 3, T, Q> const& m1, mat<3, 3, T, Q> const& m2)
	{
		return (m1[0] == m2[0]) && (m1[1] == m2[1]) && (m1[2] == m2[2]);
	}

	template<typename T, qualifier Q>
	GLM_FUNC_QUALIFIER GLM_CONSTEXPR bool operator!=(mat<3, 3, T, Q> const& m1, mat<3, 3, T, Q> const& m2)
	{
		return (m1[0] != m2[0]) || (m1[1] != m2[1]) || (m1[2] != m2[2]);
	}
} //namespace glm
