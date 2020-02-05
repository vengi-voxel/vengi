/// @ref ext_vector_int3_sized
/// @file glm/ext/vector_int3_sized.hpp
///
/// @defgroup ext_vector_int3_sized GLM_EXT_vector_int3_sized
/// @ingroup ext
///
/// Exposes sized signed integer vector of 3 components type.
///
/// Include <glm/ext/vector_int3_sized.hpp> to use the features of this extension.
///
/// @see ext_scalar_int_sized
/// @see ext_vector_uint3_sized

#pragma once

#include "../ext/vector_int3.hpp"
#include "../ext/scalar_int_sized.hpp"

#if GLM_MESSAGES == GLM_ENABLE && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_EXT_vector_int3_sized extension included")
#endif

namespace glm
{
	/// @addtogroup ext_vector_int3_sized
	/// @{

	/// Low precision 8 bit signed integer vector of 3 components type.
	typedef vec<3, int8, lowp>			lowp_i8vec3;
	/// Medium precision 8 bit signed integer vector of 3 components type.
	typedef vec<3, int8, mediump>		mediump_i8vec3;
	/// High precision 8 bit signed integer vector of 3 components type.
	typedef vec<3, int8, highp>			highp_i8vec3;
	/// 8 bit signed integer vector of 3 components type.
	typedef vec<3, int8, defaultp>		i8vec3;

	/// Low precision 16 bit signed integer vector of 3 components type.
	typedef vec<3, int16, lowp>			lowp_i16vec3;
	/// Medium precision 16 bit signed integer vector of 3 components type.
	typedef vec<3, int16, mediump>		mediump_i16vec3;
	/// High precision 16 bit signed integer vector of 3 components type.
	typedef vec<3, int16, highp>		highp_i16vec3;
	/// 16 bit signed integer vector of 3 components type.
	typedef vec<3, int16, defaultp>		i16vec3;

	/// Low precision 32 bit signed integer vector of 3 components type.
	typedef vec<3, int32, lowp>			lowp_i32vec3;
	/// Medium precision 32 bit signed integer vector of 3 components type.
	typedef vec<3, int32, mediump>		mediump_i32vec3;
	/// High precision 32 bit signed integer vector of 3 components type.
	typedef vec<3, int32, highp>		highp_i32vec3;
	/// 32 bit signed integer vector of 3 components type.
	typedef vec<3, int32, defaultp>		i32vec3;

	/// Low precision 64 bit signed integer vector of 3 components type.
	typedef vec<3, int64, lowp>			lowp_i64vec3;
	/// Medium precision 64 bit signed integer vector of 3 components type.
	typedef vec<3, int64, mediump>		mediump_i64vec3;
	/// High precision 64 bit signed integer vector of 3 components type.
	typedef vec<3, int64, highp>		highp_i64vec3;
	/// 64 bit signed integer vector of 3 components type.
	typedef vec<3, int64, defaultp>		i64vec3;

	/// @}
}//namespace glm
