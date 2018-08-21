/// @ref ext_vector_bvec1
/// @file glm/ext/vector_bool1.hpp
///
/// @see core (dependence)
///
/// @defgroup ext_vector_bool1 GLM_EXT_vector_bool1
/// @ingroup ext
///
/// Include <glm/ext/vector_bvec1.hpp> to use the features of this extension.
///
/// Exposes bvec1 vector type.

#pragma once

#include "../detail/type_vec1.hpp"

#if GLM_MESSAGES == GLM_ENABLE && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_EXT_vector_bvec1 extension included")
#endif

namespace glm
{
	/// @addtogroup ext_bvec1
	/// @{

	/// 1 components vector of boolean.
	///
	/// @see ext_vector_bvec1 extension.
	typedef vec<1, bool, defaultp>		bvec1;

	/// @}
}//namespace glm
