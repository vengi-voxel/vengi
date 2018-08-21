/// @ref ext_matrix_relational
/// @file glm/ext/matrix_relational.hpp
///
/// @see core (dependence)
///
/// @defgroup ext_vector_relational GLM_EXT_matrix_relational
/// @ingroup ext
///
/// Include <glm/ext/matrix_relational.hpp> to use the features of this extension.
///
/// Comparison functions for a user defined epsilon values.

#pragma once

// Dependencies
#include "../detail/qualifier.hpp"

#if GLM_MESSAGES == GLM_ENABLE && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_EXT_matrix_relational extension included")
#endif

namespace glm
{
	/// @addtogroup ext_matrix_relational
	/// @{

	/// Perform a component-wise equal-to comparison of two matrices.
	/// Return a boolean vector which components value is True if this expression is satisfied per column of the matrices.
	///
	/// @tparam C Integer between 1 and 4 included that qualify the number of columns of the matrix
	/// @tparam R Integer between 1 and 4 included that qualify the number of rows of the matrix
	/// @tparam T Floating-point or integer scalar types
	/// @tparam Q Value from qualifier enum
	///
	/// @see ext_matrix_relational
	template<length_t C, length_t R, typename T, qualifier Q>
	GLM_FUNC_DECL GLM_CONSTEXPR vec<C, bool, Q> equal(mat<C, R, T, Q> const& x, mat<C, R, T, Q> const& y);

	/// Returns the component-wise comparison of |x - y| < epsilon.
	/// True if this expression is satisfied.
	///
	/// @tparam C Integer between 1 and 4 included that qualify the number of columns of the matrix
	/// @tparam R Integer between 1 and 4 included that qualify the number of rows of the matrix
	/// @tparam T Floating-point or integer scalar types
	/// @tparam Q Value from qualifier enum
	///
	/// @see ext_matrix_relational
	template<length_t C, length_t R, typename T, qualifier Q>
	GLM_FUNC_DECL GLM_CONSTEXPR vec<C, bool, Q> equal(mat<C, R, T, Q> const& x, mat<C, R, T, Q> const& y, T epsilon);

	/// Returns the component-wise comparison of |x - y| < epsilon.
	/// True if this expression is satisfied.
	///
	/// @tparam C Integer between 1 and 4 included that qualify the number of columns of the matrix
	/// @tparam R Integer between 1 and 4 included that qualify the number of rows of the matrix
	/// @tparam T Floating-point or integer scalar types
	/// @tparam Q Value from qualifier enum
	///
	/// @see ext_matrix_relational
	template<length_t C, length_t R, typename T, qualifier Q>
	GLM_FUNC_DECL GLM_CONSTEXPR vec<C, bool, Q> equal(mat<C, R, T, Q> const& x, mat<C, R, T, Q> const& y, vec<C, T, Q> const& epsilon);

	/// Perform a component-wise not-equal-to comparison of two matrices.
	/// Return a boolean vector which components value is True if this expression is satisfied per column of the matrices.
	///
	/// @tparam C Integer between 1 and 4 included that qualify the number of columns of the matrix
	/// @tparam R Integer between 1 and 4 included that qualify the number of rows of the matrix
	/// @tparam T Floating-point or integer scalar types
	/// @tparam Q Value from qualifier enum
	///
	/// @see ext_matrix_relational
	template<length_t C, length_t R, typename T, qualifier Q>
	GLM_FUNC_DECL GLM_CONSTEXPR vec<C, bool, Q> notEqual(mat<C, R, T, Q> const& x, mat<C, R, T, Q> const& y);

	/// Returns the component-wise comparison of |x - y| < epsilon.
	/// True if this expression is not satisfied.
	///
	/// @tparam C Integer between 1 and 4 included that qualify the number of columns of the matrix
	/// @tparam R Integer between 1 and 4 included that qualify the number of rows of the matrix
	/// @tparam T Floating-point or integer scalar types
	/// @tparam Q Value from qualifier enum
	///
	/// @see ext_matrix_relational
	template<length_t C, length_t R, typename T, qualifier Q>
	GLM_FUNC_DECL GLM_CONSTEXPR vec<C, bool, Q> notEqual(mat<C, R, T, Q> const& x, mat<C, R, T, Q> const& y, T epsilon);

	/// Returns the component-wise comparison of |x - y| >= epsilon.
	/// True if this expression is not satisfied.
	///
	/// @tparam C Integer between 1 and 4 included that qualify the number of columns of the matrix
	/// @tparam R Integer between 1 and 4 included that qualify the number of rows of the matrix
	/// @tparam T Floating-point or integer scalar types
	/// @tparam Q Value from qualifier enum
	///
	/// @see ext_vector_relational
	template<length_t C, length_t R, typename T, qualifier Q>
	GLM_FUNC_DECL GLM_CONSTEXPR vec<C, bool, Q> notEqual(mat<C, R, T, Q> const& x, mat<C, R, T, Q> const& y, vec<C, T, Q> const& epsilon);

	/// @}
}//namespace glm

#include "matrix_relational.inl"
