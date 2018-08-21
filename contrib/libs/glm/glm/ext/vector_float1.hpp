/// @ref ext_vector_float1
/// @file glm/ext/vector_float1.hpp
///
/// @see core (dependence)
///
/// @defgroup ext_vector_float1 GLM_EXT_vector_float1
/// @ingroup ext
///
/// Include <glm/ext/vector_float1.hpp> to use the features of this extension.
///
/// Exposes vec1 vector type.

#pragma once

#include "../detail/type_vec1.hpp"

#if GLM_MESSAGES == GLM_ENABLE && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_EXT_vector_float1 extension included")
#endif

namespace glm
{
	/// @addtogroup ext_vector_float1
	/// @{

	/// 1 components vector of single-precision floating-point numbers.
	///
	/// @see ext_vector_float1 extension.
	typedef vec<1, float, defaultp>		vec1;

	/// @}
}//namespace glm
