/// @ref ext_quaternion_double_precision
/// @file glm/ext/quaternion_double_precision.hpp
///
/// @see core (dependence)
///
/// @defgroup ext_quaternion_double_precision GLM_EXT_quaternion_double_precision
/// @ingroup ext
///
/// Include <glm/ext/quaternion_double_precision.hpp> to use the features of this extension.
///
/// Defines a templated quaternion type and several quaternion operations.

#pragma once

// Dependency:
#include "../detail/type_quat.hpp"

#if GLM_MESSAGES == GLM_ENABLE && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_EXT_quaternion_double_precision extension included")
#endif

namespace glm
{
	/// @addtogroup ext_quaternion_double_precision
	/// @{

	/// Quaternion of double-precision floating-point numbers using high precision arithmetic in term of ULPs.
	///
	/// @see ext_quaternion_double_precision
	typedef qua<double, lowp>		lowp_dquat;

	/// Quaternion of medium double-qualifier floating-point numbers using high precision arithmetic in term of ULPs.
	///
	/// @see ext_quaternion_double_precision
	typedef qua<double, mediump>	mediump_dquat;

	/// Quaternion of high double-qualifier floating-point numbers using high precision arithmetic in term of ULPs.
	///
	/// @see ext_quaternion_double_precision
	typedef qua<double, highp>		highp_dquat;

	/// @}
} //namespace glm

