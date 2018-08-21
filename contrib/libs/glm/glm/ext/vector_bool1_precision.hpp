/// @ref ext_vector_bvec1_precision
/// @file glm/ext/vector_bvec1_precision.hpp
///
/// @see core (dependence)
///
/// @defgroup ext_vector_bvec1_precision GLM_EXT_vector_bvec1_precision
/// @ingroup ext
///
/// Include <glm/ext/vector_bvec1_precision.hpp> to use the features of this extension.
///
/// Exposes highp_bvec1, mediump_bvec1 and lowp_bvec1 types.

#pragma once

#include "../detail/type_vec1.hpp"

#if GLM_MESSAGES == GLM_ENABLE && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_EXT_vector_bvec1_precision extension included")
#endif

namespace glm
{
	/// @addtogroup ext_vector_bvec1_precision
	/// @{

	/// 1 component vector of bool values.
	///
	/// @see ext_vector_bvec1_precision
	typedef vec<1, bool, highp>			highp_bvec1;

	/// 1 component vector of bool values.
	///
	/// @see ext_vector_bvec1_precision
	typedef vec<1, bool, mediump>		mediump_bvec1;

	/// 1 component vector of bool values.
	///
	/// @see ext_vector_bvec1_precision
	typedef vec<1, bool, lowp>			lowp_bvec1;

	/// @}
}//namespace glm
