/**
 * @file
 */

#include "Util.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/Assert.h"
#include "core/ArrayLength.h"
#include "video/Shader.h"
#include "video/Version.h"

namespace util {

static const Types cTypes[] = {
	{ Variable::FLOAT,                 1,  1, 1, "float",        Value,     "float" },
	{ Variable::UNSIGNED_INT,          1,  1, 1, "uint32_t",     Value,     "uint" },
	{ Variable::INT,                   1,  1, 1, "int32_t",      Value,     "int" },
	{ Variable::UVEC2,                 2,  2, 2, "glm::highp_uvec2",   Reference, "uvec2" },
	{ Variable::UVEC3,                 3,  4, 3, "glm::highp_uvec3",   Reference, "uvec3" },
	{ Variable::UVEC4,                 4,  4, 4, "glm::highp_uvec4",   Reference, "uvec4" },
	{ Variable::IVEC2,                 2,  2, 2, "glm::highp_ivec2",   Reference, "ivec2" },
	{ Variable::IVEC3,                 3,  4, 3, "glm::highp_ivec3",   Reference, "ivec3" },
	{ Variable::IVEC4,                 4,  4, 4, "glm::highp_ivec4",   Reference, "ivec4" },
	{ Variable::VEC2,                  2,  2, 2, "glm::highp_vec2",    Reference, "vec2" },
	{ Variable::VEC3,                 3,  4, 3, "glm::highp_vec3",    Reference, "vec3" },
	{ Variable::VEC4,                 4,  4, 4, "glm::highp_vec4",    Reference, "vec4" },
	{ Variable::MAT4,                 16, 4, 1, "glm::highp_mat4",    Reference, "mat4" },

	{ Variable::IMAGE2D,              -1, -1, 1, "video::TextureUnit", Value, "image2D" },
	{ Variable::SAMPLER1D,            -1, -1, 1, "video::TextureUnit", Value, "sampler1D" },
	{ Variable::SAMPLER2D,            -1, -1, 1, "video::TextureUnit", Value, "sampler2D" },
	{ Variable::SAMPLER2DARRAY,       -1, -1, 1, "video::TextureUnit", Value, "sampler2DArray" },
	{ Variable::SAMPLER2DARRAYSHADOW, -1, -1, 1, "video::TextureUnit", Value, "sampler2DArrayShadow" },
	{ Variable::SAMPLER3D,            -1, -1, 1, "video::TextureUnit", Value, "sampler3D" },
	{ Variable::SAMPLER2DMS,          -1, -1, 1, "video::TextureUnit", Value, "sampler2DMS" },
	{ Variable::SAMPLERCUBEMAP,       -1, -1, 1, "video::TextureUnit", Value, "samplerCube" },
	{ Variable::SAMPLER1DSHADOW,      -1, -1, 1, "video::TextureUnit", Value, "sampler1DShadow" },
	{ Variable::SAMPLER2DSHADOW,      -1, -1, 1, "video::TextureUnit", Value, "sampler2DShadow" },
	{ Variable::USAMPLER3D,           -1, -1, 1, "video::TextureUnit", Value, "usampler3D" }
};
static_assert(Variable::MAX == lengthof(cTypes), "mismatch in glsl types");

int getComponents(const Variable::Type type) {
	return resolveTypes(type).components;
}

Variable::Type getType(const core::String& type, int line) {
	int max = core::enumVal(Variable::MAX);
	for (int i = 0; i < max; ++i) {
		if (type == cTypes[i].glsltype) {
			return cTypes[i].type;
		}
	}
	Log::error("Unknown type given: %s at line %i - assuming float", type.c_str(), line);
	core_assert_msg(false, "Unknown type given: %s at line %i - assuming float", type.c_str(), line);
	return Variable::FLOAT;
}

#define IMAGEFORMATENTRY(x) {video::ImageFormat::x, #x, "GL_" #x}
static const ImageFormatType cImageFormat[] = {
	IMAGEFORMATENTRY(RGBA32F),
	IMAGEFORMATENTRY(RGBA16F),
	IMAGEFORMATENTRY(RG32F),
	IMAGEFORMATENTRY(RG16F),
	IMAGEFORMATENTRY(R11F_G11F_B10F),
	IMAGEFORMATENTRY(R32F),
	IMAGEFORMATENTRY(R16F),
	IMAGEFORMATENTRY(RGBA16),
	IMAGEFORMATENTRY(RGB10_A2),
	IMAGEFORMATENTRY(RGBA8),
	IMAGEFORMATENTRY(RG16),
	IMAGEFORMATENTRY(RG8),
	IMAGEFORMATENTRY(R16),
	IMAGEFORMATENTRY(R8),
	IMAGEFORMATENTRY(RGBA16_SNORM),
	IMAGEFORMATENTRY(RGBA8_SNORM),
	IMAGEFORMATENTRY(RG16_SNORM),
	IMAGEFORMATENTRY(RG8_SNORM),
	IMAGEFORMATENTRY(R16_SNORM),
	IMAGEFORMATENTRY(R8_SNORM),
	IMAGEFORMATENTRY(RGBA32I),
	IMAGEFORMATENTRY(RGBA16I),
	IMAGEFORMATENTRY(RGBA8I),
	IMAGEFORMATENTRY(RG32I),
	IMAGEFORMATENTRY(RG16I),
	IMAGEFORMATENTRY(RG8I),
	IMAGEFORMATENTRY(R32I),
	IMAGEFORMATENTRY(R16I),
	IMAGEFORMATENTRY(R8I),
	IMAGEFORMATENTRY(RGBA32UI),
	IMAGEFORMATENTRY(RGBA16UI),
	IMAGEFORMATENTRY(RGB10_A2UI),
	IMAGEFORMATENTRY(RGBA8UI),
	IMAGEFORMATENTRY(RG32UI),
	IMAGEFORMATENTRY(RG16UI),
	IMAGEFORMATENTRY(RG8UI),
	IMAGEFORMATENTRY(R32UI),
	IMAGEFORMATENTRY(R16UI),
	IMAGEFORMATENTRY(R8UI)
};
#undef IMAGEFORMATENTRY
static_assert((int)video::ImageFormat::Max == lengthof(cImageFormat), "mismatch in image formats");

video::ImageFormat getImageFormat(const core::String& glslType, int line) {
	const int max = core::enumVal(video::ImageFormat::Max);
	const core::String& upper = glslType.toUpper();
	for (int i = 0; i < max; ++i) {
		if (upper == cImageFormat[i].glsltype) {
			return cImageFormat[i].type;
		}
	}
	return video::ImageFormat::Max;
}

const char* getImageFormatGLType(video::ImageFormat format) {
	const int max = core::enumVal(video::ImageFormat::Max);
	for (int i = 0; i < max; ++i) {
		if (format == cImageFormat[i].type) {
			return cImageFormat[i].ctype;
		}
	}
	return nullptr;
}

const char* getImageFormatTypeString(video::ImageFormat format) {
	const int max = core::enumVal(video::ImageFormat::Max);
	for (int i = 0; i < max; ++i) {
		if (format == cImageFormat[i].type) {
			return cImageFormat[i].glsltype;
		}
	}
	return nullptr;
}

#define PRIMITVEENTRY(x) {video::Primitive::x, #x}
static const PrimitiveType cPrimitiveType[] = {
	PRIMITVEENTRY(Points),
	PRIMITVEENTRY(Lines),
	PRIMITVEENTRY(LinesAdjacency),
	PRIMITVEENTRY(Triangles),
	PRIMITVEENTRY(TrianglesAdjacency),
	PRIMITVEENTRY(LineStrip),
	PRIMITVEENTRY(TriangleStrip)
};
#undef PRIMITVEENTRY
static_assert((int)video::Primitive::Max == lengthof(cPrimitiveType), "mismatch in primitive types");

const char* getPrimitiveTypeString(video::Primitive primitive) {
	const int max = core::enumVal(video::Primitive::Max);
	for (int i = 0; i < max; ++i) {
		if (primitive == cPrimitiveType[i].type) {
			return cPrimitiveType[i].str;
		}
	}
	return nullptr;
}

core::String uniformSetterPostfix(const Variable::Type type, int amount) {
	switch (type) {
	case Variable::IMAGE2D:
	case Variable::SAMPLER1D:
	case Variable::SAMPLER2D:
	case Variable::SAMPLER2DMS:
	case Variable::SAMPLER3D:
	case Variable::SAMPLER1DSHADOW:
	case Variable::SAMPLER2DSHADOW:
	case Variable::SAMPLER2DARRAY:
	case Variable::SAMPLER2DARRAYSHADOW:
	case Variable::USAMPLER3D:
	case Variable::SAMPLERCUBEMAP:
		if (amount > 1) {
			// https://www.opengl.org/wiki/Data_Type_%28GLSL%29#Opaque_arrays
			if (video::Shader::glslVersion < video::GLSLVersion::V400) {
				Log::warn("Sampler arrays are only allowed under special circumstances - don't do this for GLSL < 4.0");
			}
			// TODO: doesn't work yet, video::TextureUnit support is needed here
			return "1iv";
		}
		return core::String::Empty;
	default:
		break;
	}
	return core::String::Empty;
}

core::String convertName(const core::String& in, bool firstUpper) {
	core::String out;
	core::DynamicArray<core::String> nameParts;
	core::string::splitString(in, nameParts, "_-");
	for (core::String& n : nameParts) {
		if (n.size() > 1 || nameParts.size() < 2) {
			if (!firstUpper) {
				firstUpper = true;
			} else {
				n[0] = core::string::toUpper(n[0]);
			}
			out += n;
		}
	}
	if (out.empty()) {
		out = in;
	}
	return out;
}

/**
 * The rules for std140 layout are covered quite well in the OpenGL specification (OpenGL 4.5, Section 7.6.2.2, page
 * 137). Among the most important is the fact that arrays of types are not necessarily tightly packed. An array of
 * floats in such a block will not be the equivalent to an array of floats in C/C++. The array stride (the bytes between
 * array elements) is always rounded up to the size of a vec4 (ie: 16-bytes). So arrays will only match their C/C++
 * definitions if the type is a multiple of 16 bytes
 */
size_t std140Size(const Variable& v) {
	const Types& cType = resolveTypes(v.type);
	if (v.arraySize > 0) {
		return (size_t)cType.size * v.arraySize;
	}
	return cType.size;
}

int std140Align(const Variable& v) {
	const Types& cType = resolveTypes(v.type);
	return cType.align;
}

/**
 * std430 layout rules (OpenGL 4.3+, Section 7.6.2.2):
 * - Scalars: natural alignment (4 bytes for float/int/uint)
 * - vec2: 2N alignment (8 bytes)
 * - vec3, vec4: 4N alignment (16 bytes)
 * - Arrays: element stride is the element size rounded to alignment (NOT always vec4 like std140)
 * - Structs: alignment is the largest alignment of any member
 *
 * Key difference from std140: arrays of scalars/vec2 don't need vec4 padding between elements
 */
size_t std430Size(const Variable& v) {
	const Types& cType = resolveTypes(v.type);
	if (v.arraySize > 0) {
		return (size_t)cType.size * v.arraySize;
	}
	return cType.size;
}

int std430Align(const Variable& v) {
	const Types& cType = resolveTypes(v.type);
	// std430 has the same alignment rules as std140 for individual types
	// The difference is mainly in array stride calculation
	return cType.align;
}

const Types& resolveTypes(Variable::Type type) {
	int max = core::enumVal(Variable::MAX);
	for (int i = 0; i < max; ++i) {
		if (type == cTypes[i].type) {
			return cTypes[i];
		}
	}
	Log::error("Unknown type given: assuming first entry");
	core_assert_msg(false, "Unknown type given: %i", int(type));
	return cTypes[0];
}

}
