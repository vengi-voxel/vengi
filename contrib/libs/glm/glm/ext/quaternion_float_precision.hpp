/// @ref ext_quaternion_float_precision
/// @file glm/ext/quaternion_float_precision.hpp
///
/// @see core (dependence)
///
/// @defgroup ext_quaternion_float_precision GLM_EXT_quaternion_float_precision
/// @ingroup ext
///
/// Include <glm/ext/quaternion_float_precision.hpp> to use the features of this extension.
///
/// Defines a templated quaternion type and several quaternion operations.

#pragma once

// Dependency:
#include "../detail/type_quat.hpp"

#if GLM_MESSAGES == GLM_ENABLE && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_EXT_quaternion_float_precision extension included")
#endif

namespace glm
{
	/// @addtogroup ext_quaternion_float_precision
	/// @{

	/// Quaternion of single-precision floating-point numbers using high precision arithmetic in term of ULPs.
	///
	/// @see ext_quaternion_float_precision
	typedef qua<float, lowp>		lowp_quat;

	/// Quaternion of single-precision floating-point numbers using high precision arithmetic in term of ULPs.
	///
	/// @see ext_quaternion_float_precision
	typedef qua<float, mediump>		mediump_quat;

	/// Quaternion of single-precision floating-point numbers using high precision arithmetic in term of ULPs.
	///
	/// @see ext_quaternion_float_precision
	typedef qua<float, highp>		highp_quat;

	/// @}
} //namespace glm

