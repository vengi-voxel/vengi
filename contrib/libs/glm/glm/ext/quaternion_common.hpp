/// @ref ext_quaternion_common
/// @file glm/ext/quaternion_common.hpp
///
/// @see core (dependence)
///
/// @defgroup ext_quaternion_common GLM_EXT_quaternion_common
/// @ingroup ext
///
/// Include <glm/ext/quaternion_common.hpp> to use the features of this extension.
///
/// Defines a templated quaternion type and several quaternion operations.

#pragma once

// Dependency:
#include "../ext/scalar_constants.hpp"
#include "../ext/quaternion_geometric.hpp"
#include "../common.hpp"
#include "../trigonometric.hpp"
#include "../exponential.hpp"
#include <limits>

#if GLM_MESSAGES == GLM_ENABLE && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_EXT_quaternion_common extension included")
#endif

namespace glm
{
	/// @addtogroup ext_quaternion_common
	/// @{

	/// Spherical linear interpolation of two quaternions.
	/// The interpolation is oriented and the rotation is performed at constant speed.
	/// For short path spherical linear interpolation, use the slerp function.
	///
	/// @param x A quaternion
	/// @param y A quaternion
	/// @param a Interpolation factor. The interpolation is defined beyond the range [0, 1].
	/// @tparam T Floating-point scalar types.
	///
	/// @see - slerp(qua<T, Q> const& x, qua<T, Q> const& y, T const& a)
	/// @see ext_quaternion_common
	template<typename T, qualifier Q>
	GLM_FUNC_DECL qua<T, Q> mix(qua<T, Q> const& x, qua<T, Q> const& y, T a);

	/// Linear interpolation of two quaternions.
	/// The interpolation is oriented.
	///
	/// @param x A quaternion
	/// @param y A quaternion
	/// @param a Interpolation factor. The interpolation is defined in the range [0, 1].
	/// @tparam T Floating-point scalar types.
	///
	/// @see ext_quaternion_common
	template<typename T, qualifier Q>
	GLM_FUNC_DECL qua<T, Q> lerp(qua<T, Q> const& x, qua<T, Q> const& y, T a);

	/// Spherical linear interpolation of two quaternions.
	/// The interpolation always take the short path and the rotation is performed at constant speed.
	///
	/// @param x A quaternion
	/// @param y A quaternion
	/// @param a Interpolation factor. The interpolation is defined beyond the range [0, 1].
	/// @tparam T Floating-point scalar types.
	///
	/// @see ext_quaternion_common
	template<typename T, qualifier Q>
	GLM_FUNC_DECL qua<T, Q> slerp(qua<T, Q> const& x, qua<T, Q> const& y, T a);

	/// Returns the q conjugate.
	///
	/// @tparam T Floating-point scalar types.
	///
	/// @see ext_quaternion_common
	template<typename T, qualifier Q>
	GLM_FUNC_DECL qua<T, Q> conjugate(qua<T, Q> const& q);

	/// Returns the q inverse.
	///
	/// @tparam T Floating-point scalar types.
	///
	/// @see ext_quaternion_common
	template<typename T, qualifier Q>
	GLM_FUNC_DECL qua<T, Q> inverse(qua<T, Q> const& q);

	/// Returns true if x holds a NaN (not a number)
	/// representation in the underlying implementation's set of
	/// floating point representations. Returns false otherwise,
	/// including for implementations with no NaN
	/// representations.
	///
	/// /!\ When using compiler fast math, this function may fail.
	///
	/// @tparam T Floating-point scalar types.
	///
	/// @see ext_quaternion_common
	template<typename T, qualifier Q>
	GLM_FUNC_DECL vec<4, bool, Q> isnan(qua<T, Q> const& x);

	/// Returns true if x holds a positive infinity or negative
	/// infinity representation in the underlying implementation's
	/// set of floating point representations. Returns false
	/// otherwise, including for implementations with no infinity
	/// representations.
	///
	/// @tparam T Floating-point scalar types.
	///
	/// @see ext_quaternion_common
	template<typename T, qualifier Q>
	GLM_FUNC_DECL vec<4, bool, Q> isinf(qua<T, Q> const& x);

	/// @}
} //namespace glm

#include "quaternion_common.inl"
