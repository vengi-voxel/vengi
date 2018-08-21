/// @ref ext_quaternion_double
/// @file glm/ext/quaternion_double.hpp
///
/// @see core (dependence)
///
/// @defgroup ext_quaternion_double GLM_EXT_quaternion_double
/// @ingroup ext
///
/// Include <glm/ext/quaternion_double.hpp> to use the features of this extension.
///
/// Defines a templated quaternion type and several quaternion operations.

#pragma once

// Dependency:
#include "../detail/type_quat.hpp"

#if GLM_MESSAGES == GLM_ENABLE && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_EXT_quaternion_double extension included")
#endif

namespace glm
{
	/// @addtogroup ext_quaternion_double
	/// @{

	/// Quaternion of double-precision floating-point numbers.
	///
	/// @see ext_quaternion_double
	typedef qua<double, defaultp>		dquat;

	/// @}
} //namespace glm

