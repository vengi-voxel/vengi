/// @ref ext_scalar_double
/// @file glm/ext/scalar_double.hpp
///
/// @see core (dependence)
///
/// @defgroup ext_scalar_double GLM_EXT_scalar_double
/// @ingroup ext
///
/// Include <glm/ext/scalar_double.hpp> to use the features of this extension.
///
/// Exposes double scalar type.

#pragma once

#include "setup.hpp"

namespace glm{
namespace detail
{
	typedef float				float32;

#	ifndef GLM_FORCE_SINGLE_ONLY
		typedef double			float64;
#	endif//GLM_FORCE_SINGLE_ONLY
}//namespace detail

	typedef float				lowp_float_t;
	typedef float				mediump_float_t;
	typedef double				highp_float_t;

	/// @addtogroup core_precision
	/// @{

	/// Low qualifier floating-point numbers.
	/// There is no guarantee on the actual qualifier.
	///
	/// @see <a href="http://www.opengl.org/registry/doc/GLSLangSpec.4.20.8.pdf">GLSL 4.20.8 specification, section 4.1.4 Floats</a>
	/// @see <a href="http://www.opengl.org/registry/doc/GLSLangSpec.4.20.8.pdf">GLSL 4.20.8 specification, section 4.7.2 Precision Qualifier</a>
	typedef lowp_float_t		lowp_float;

	/// Medium qualifier floating-point numbers.
	/// There is no guarantee on the actual qualifier.
	///
	/// @see <a href="http://www.opengl.org/registry/doc/GLSLangSpec.4.20.8.pdf">GLSL 4.20.8 specification, section 4.1.4 Floats</a>
	/// @see <a href="http://www.opengl.org/registry/doc/GLSLangSpec.4.20.8.pdf">GLSL 4.20.8 specification, section 4.7.2 Precision Qualifier</a>
	typedef mediump_float_t		mediump_float;

	/// High qualifier floating-point numbers.
	/// There is no guarantee on the actual qualifier.
	///
	/// @see <a href="http://www.opengl.org/registry/doc/GLSLangSpec.4.20.8.pdf">GLSL 4.20.8 specification, section 4.1.4 Floats</a>
	/// @see <a href="http://www.opengl.org/registry/doc/GLSLangSpec.4.20.8.pdf">GLSL 4.20.8 specification, section 4.7.2 Precision Qualifier</a>
	typedef highp_float_t		highp_float;

#if GLM_CONFIG_PRECISION_FLOAT == GLM_HIGHP
	typedef highp_float			float_t;
#elif GLM_CONFIG_PRECISION_FLOAT == GLM_MEDIUMP
	typedef mediump_float		float_t;
#elif GLM_CONFIG_PRECISION_FLOAT == GLM_LOWP
	typedef lowp_float			float_t;
#endif

	typedef float				float32;

#	ifndef GLM_FORCE_SINGLE_ONLY
		typedef double			float64;
#	endif//GLM_FORCE_SINGLE_ONLY

////////////////////
// check type sizes
#	ifndef GLM_FORCE_SINGLE_ONLY
		GLM_STATIC_ASSERT(sizeof(glm::float64) == 8, "float64 size isn't 8 bytes on this platform");
#	endif//GLM_FORCE_SINGLE_ONLY

	/// @}

}//namespace glm
