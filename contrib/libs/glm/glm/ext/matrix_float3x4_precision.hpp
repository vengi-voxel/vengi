/// @ref core
/// @file glm/ext/matrix_float3x4_precision.hpp

#pragma once
#include "../detail/type_mat3x4.hpp"

namespace glm
{
	/// @addtogroup core_matrix_precision
	/// @{

	/// 3 columns of 4 components matrix of single-precision floating-point numbers using low precision arithmetic in term of ULPs.
	///
	/// @see <a href="http://www.opengl.org/registry/doc/GLSLangSpec.4.20.8.pdf">GLSL 4.20.8 specification, section 4.1.6 Matrices</a>
	/// @see <a href="http://www.opengl.org/registry/doc/GLSLangSpec.4.20.8.pdf">GLSL 4.20.8 specification, section 4.7.2 Precision Qualifier</a>
	typedef mat<3, 4, float, lowp>		lowp_mat3x4;

	/// 3 columns of 4 components matrix of single-precision floating-point numbers using medium precision arithmetic in term of ULPs.
	///
	/// @see <a href="http://www.opengl.org/registry/doc/GLSLangSpec.4.20.8.pdf">GLSL 4.20.8 specification, section 4.1.6 Matrices</a>
	/// @see <a href="http://www.opengl.org/registry/doc/GLSLangSpec.4.20.8.pdf">GLSL 4.20.8 specification, section 4.7.2 Precision Qualifier</a>
	typedef mat<3, 4, float, mediump>	mediump_mat3x4;

	/// 3 columns of 4 components matrix of single-precision floating-point numbers using high precision arithmetic in term of ULPs.
	///
	/// @see <a href="http://www.opengl.org/registry/doc/GLSLangSpec.4.20.8.pdf">GLSL 4.20.8 specification, section 4.1.6 Matrices</a>
	/// @see <a href="http://www.opengl.org/registry/doc/GLSLangSpec.4.20.8.pdf">GLSL 4.20.8 specification, section 4.7.2 Precision Qualifier</a>
	typedef mat<3, 4, float, highp>		highp_mat3x4;

	/// @}
}//namespace glm

#if GLM_CONFIG_CTOR_INIT == GLM_DISABLE
static_assert(std::is_trivially_default_constructible<glm::lowp_mat3x4>::value);
static_assert(std::is_trivially_default_constructible<glm::mediump_mat3x4>::value);
static_assert(std::is_trivially_default_constructible<glm::highp_mat3x4>::value);
#endif
static_assert(std::is_trivially_copy_assignable<glm::lowp_mat3x4>::value);
static_assert(std::is_trivially_copy_assignable<glm::mediump_mat3x4>::value);
static_assert(std::is_trivially_copy_assignable<glm::highp_mat3x4>::value);
static_assert(std::is_trivially_copyable<glm::lowp_mat3x4>::value);
static_assert(std::is_trivially_copyable<glm::mediump_mat3x4>::value);
static_assert(std::is_trivially_copyable<glm::highp_mat3x4>::value);
static_assert(std::is_copy_constructible<glm::lowp_mat3x4>::value);
static_assert(std::is_copy_constructible<glm::mediump_mat3x4>::value);
static_assert(std::is_copy_constructible<glm::highp_mat3x4>::value);
static_assert(glm::lowp_mat3x4::length() == 3);
static_assert(glm::mediump_mat3x4::length() == 3);
static_assert(glm::highp_mat3x4::length() == 3);
