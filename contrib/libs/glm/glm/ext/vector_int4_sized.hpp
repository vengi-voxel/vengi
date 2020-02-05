/// @ref ext_vector_int4_sized
/// @file glm/ext/vector_int4_sized.hpp
///
/// @defgroup ext_vector_int4_sized GLM_EXT_vector_int4_sized
/// @ingroup ext
///
/// Exposes sized signed integer vector of 4 components type.
///
/// Include <glm/ext/vector_int4_sized.hpp> to use the features of this extension.
///
/// @see ext_scalar_int_sized
/// @see ext_vector_uint4_sized

#pragma once

#include "../ext/vector_int4.hpp"
#include "../ext/scalar_int_sized.hpp"

#if GLM_MESSAGES == GLM_ENABLE && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_EXT_vector_int4_sized extension included")
#endif

namespace glm
{
	/// @addtogroup ext_vector_int4_sized
	/// @{

	/// Low precision 8 bit signed integer vector of 4 components type.
	typedef vec<4, int8, lowp>			lowp_i8vec4;
	/// Medium precision 8 bit signed integer vector of 4 components type.
	typedef vec<4, int8, mediump>		mediump_i8vec4;
	/// High precision 8 bit signed integer vector of 4 components type.
	typedef vec<4, int8, highp>			highp_i8vec4;
	/// 8 bit signed integer vector of 4 components type.
	typedef vec<4, int8, defaultp>		i8vec4;

	/// Low precision 16 bit signed integer vector of 4 components type.
	typedef vec<4, int16, lowp>			lowp_i16vec4;
	/// Medium precision 16 bit signed integer vector of 4 components type.
	typedef vec<4, int16, mediump>		mediump_i16vec4;
	/// High precision 16 bit signed integer vector of 4 components type.
	typedef vec<4, int16, highp>		highp_i16vec4;
	/// 16 bit signed integer vector of 4 components type.
	typedef vec<4, int16, defaultp>		i16vec4;

	/// Low precision 32 bit signed integer vector of 4 components type.
	typedef vec<4, int32, lowp>			lowp_i32vec4;
	/// Medium precision 32 bit signed integer vector of 4 components type.
	typedef vec<4, int32, mediump>		mediump_i32vec4;
	/// High precision 32 bit signed integer vector of 4 components type.
	typedef vec<4, int32, highp>		highp_i32vec4;
	/// 32 bit signed integer vector of 4 components type.
	typedef vec<4, int32, defaultp>		i32vec4;

	/// Low precision 64 bit signed integer vector of 4 components type.
	typedef vec<4, int64, lowp>			lowp_i64vec4;
	/// Medium precision 64 bit signed integer vector of 4 components type.
	typedef vec<4, int64, mediump>		mediump_i64vec4;
	/// High precision 64 bit signed integer vector of 4 components type.
	typedef vec<4, int64, highp>		highp_i64vec4;
	/// 64 bit signed integer vector of 4 components type.
	typedef vec<4, int64, defaultp>		i64vec4;

	/// @}
}//namespace glm
