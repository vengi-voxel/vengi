/// @ref ext_vector_double1
/// @file glm/ext/vector_double1.hpp
///
/// @see core (dependence)
///
/// @defgroup ext_vector_dvec1 GLM_EXT_vector_double1
/// @ingroup ext
///
/// Include <glm/ext/vector_double1.hpp> to use the features of this extension.
///
/// Expose dvec1 vector type.

#pragma once

#include "../detail/type_vec1.hpp"

#if GLM_MESSAGES == GLM_ENABLE && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_EXT_vector_dvec1 extension included")
#endif

namespace glm
{
	/// @addtogroup ext_vector_dvec1
	/// @{

	/// 1 components vector of double-precision floating-point numbers.
	///
	/// @see ext_vector_dvec1 extension.
	typedef vec<1, double, defaultp>		dvec1;

	/// @}
}//namespace glm
