/// @ref ext_vector_uint3_sized
/// @file glm/ext/vector_uint3_sized.hpp
///
/// @defgroup ext_vector_uint3_sized GLM_EXT_vector_uint3_sized
/// @ingroup ext
///
/// Exposes sized unsigned integer vector of 3 components type.
///
/// Include <glm/ext/vector_uint3_sized.hpp> to use the features of this extension.
///
/// @see ext_scalar_uint_sized
/// @see ext_vector_int3_sized

#pragma once

#include "../ext/vector_uint3.hpp"
#include "../ext/scalar_uint_sized.hpp"

#if GLM_MESSAGES == GLM_ENABLE && !defined(GLM_EXT_INCLUDED)
#	pragma message("GLM: GLM_EXT_vector_uint3_sized extension included")
#endif

namespace glm
{
	/// @addtogroup ext_vector_uint3_sized
	/// @{

	/// Low precision 8 bit unsigned integer vector of 3 components type.
	typedef vec<3, uint8, lowp>			lowp_u8vec3;
	/// Medium precision 8 bit unsigned integer vector of 3 components type.
	typedef vec<3, uint8, mediump>		mediump_u8vec3;
	/// High precision 8 bit unsigned integer vector of 3 components type.
	typedef vec<3, uint8, highp>		highp_u8vec3;
	/// 8 bit unsigned integer vector of 3 components type.
	typedef vec<3, uint8, defaultp>		u8vec3;

	/// Low precision 16 bit unsigned integer vector of 3 components type.
	typedef vec<3, uint16, lowp>		lowp_u16vec3;
	/// Medium precision 16 bit unsigned integer vector of 3 components type.
	typedef vec<3, uint16, mediump>		mediump_u16vec3;
	/// High precision 16 bit unsigned integer vector of 3 components type.
	typedef vec<3, uint16, highp>		highp_u16vec3;
	/// 16 bit unsigned integer vector of 3 components type.
	typedef vec<3, uint16, defaultp>	u16vec3;

	/// Low precision 32 bit unsigned integer vector of 3 components type.
	typedef vec<3, uint32, lowp>		lowp_u32vec3;
	/// Medium precision 32 bit unsigned integer vector of 3 components type.
	typedef vec<3, uint32, mediump>		mediump_u32vec3;
	/// High precision 32 bit unsigned integer vector of 3 components type.
	typedef vec<3, uint32, highp>		highp_u32vec3;
	/// 32 bit unsigned integer vector of 3 components type.
	typedef vec<3, uint32, defaultp>	u32vec3;

	/// Low precision 64 bit unsigned integer vector of 3 components type.
	typedef vec<3, uint64, lowp>		lowp_u64vec3;
	/// Medium precision 64 bit unsigned integer vector of 3 components type.
	typedef vec<3, uint64, mediump>		mediump_u64vec3;
	/// High precision 64 bit unsigned integer vector of 3 components type.
	typedef vec<3, uint64, highp>		highp_u64vec3;
	/// 64 bit unsigned integer vector of 3 components type.
	typedef vec<3, uint64, defaultp>	u64vec3;

	/// @}
}//namespace glm
