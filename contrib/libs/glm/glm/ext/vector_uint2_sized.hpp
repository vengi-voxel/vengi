/// @ref ext_vector_uint2_sized
/// @file glm/ext/vector_uint2_sized.hpp
///
/// @defgroup ext_vector_uint2_sized GLM_EXT_vector_uint2_sized
/// @ingroup ext
///
/// Exposes sized unsigned integer vector of 2 components type.
///
/// Include <glm/ext/vector_uint2_sized.hpp> to use the features of this extension.
///
/// @see ext_scalar_uint_sized
/// @see ext_vector_int2_sized

#pragma once

#include "../ext/vector_uint2.hpp"
#include "../ext/scalar_uint_sized.hpp"

#if GLM_MESSAGES == GLM_ENABLE && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_EXT_vector_uint2_sized extension included")
#endif

namespace glm
{
	/// @addtogroup ext_vector_uint2_sized
	/// @{

	/// Low precision 8 bit unsigned integer vector of 2 components type.
	typedef vec<2, uint8, lowp>			lowp_u8vec2;
	/// Medium precision 8 bit unsigned integer vector of 2 components type.
	typedef vec<2, uint8, mediump>		mediump_u8vec2;
	/// High precision 8 bit unsigned integer vector of 2 components type.
	typedef vec<2, uint8, highp>		highp_u8vec2;
	/// 8 bit unsigned integer vector of 2 components type.
	typedef vec<2, uint8, defaultp>		u8vec2;

	/// Low precision 16 bit unsigned integer vector of 2 components type.
	typedef vec<2, uint16, lowp>		lowp_u16vec2;
	/// Medium precision 16 bit unsigned integer vector of 2 components type.
	typedef vec<2, uint16, mediump>		mediump_u16vec2;
	/// High precision 16 bit unsigned integer vector of 2 components type.
	typedef vec<2, uint16, highp>		highp_u16vec2;
	/// 16 bit unsigned integer vector of 2 components type.
	typedef vec<2, uint16, defaultp>	u16vec2;

	/// Low precision 32 bit unsigned integer vector of 2 components type.
	typedef vec<2, uint32, lowp>		lowp_u32vec2;
	/// Medium precision 32 bit unsigned integer vector of 2 components type.
	typedef vec<2, uint32, mediump>		mediump_u32vec2;
	/// High precision 32 bit unsigned integer vector of 2 components type.
	typedef vec<2, uint32, highp>		highp_u32vec2;
	/// 32 bit unsigned integer vector of 2 components type.
	typedef vec<2, uint32, defaultp>	u32vec2;

	/// Low precision 64 bit unsigned integer vector of 2 components type.
	typedef vec<2, uint64, lowp>		lowp_u64vec2;
	/// Medium precision 64 bit unsigned integer vector of 2 components type.
	typedef vec<2, uint64, mediump>		mediump_u64vec2;
	/// High precision 64 bit unsigned integer vector of 2 components type.
	typedef vec<2, uint64, highp>		highp_u64vec2;
	/// 64 bit unsigned integer vector of 2 components type.
	typedef vec<2, uint64, defaultp>	u64vec2;

	/// @}
}//namespace glm
