/**
 * @file
 */

#include "Util.h"
#include "core/String.h"
#include "core/Assert.h"
#include "core/Array.h"
#include <vector>
#include <SDL.h>
#include "video/Shader.h"
#include "video/Version.h"

namespace util {

static const Types cTypes[] = {
	{ Variable::DOUBLE,          1, "double",       Value,     "double" },
	{ Variable::FLOAT,           1, "float",        Value,     "float" },
	{ Variable::UNSIGNED_INT,    1, "uint32_t",     Value,     "uint" },
	{ Variable::INT,             1, "int32_t",      Value,     "int" },
	{ Variable::BOOL,            1, "bool",         Value,     "bool" },
	{ Variable::BVEC2,           2, "glm::bvec2",   Reference, "bvec2" },
	{ Variable::BVEC3,           3, "glm::bvec3",   Reference, "bvec3" },
	{ Variable::BVEC4,           4, "glm::bvec4",   Reference, "bvec4" },
	{ Variable::DVEC2,           2, "glm::dvec2",   Reference, "dvec2" },
	{ Variable::DVEC3,           3, "glm::dvec3",   Reference, "dvec3" },
	{ Variable::DVEC4,           4, "glm::dvec4",   Reference, "dvec4" },
	{ Variable::UVEC2,           2, "glm::uvec2",   Reference, "uvec2" },
	{ Variable::UVEC3,           3, "glm::uvec3",   Reference, "uvec3" },
	{ Variable::UVEC4,           4, "glm::uvec4",   Reference, "uvec4" },
	{ Variable::IVEC2,           2, "glm::ivec2",   Reference, "ivec2" },
	{ Variable::IVEC3,           3, "glm::ivec3",   Reference, "ivec3" },
	{ Variable::IVEC4,           4, "glm::ivec4",   Reference, "ivec4" },
	{ Variable::VEC2,            2, "glm::vec2",    Reference, "vec2" },
	{ Variable::VEC3,            3, "glm::vec3",    Reference, "vec3" },
	{ Variable::VEC4,            4, "glm::vec4",    Reference, "vec4" },
	{ Variable::MAT2,            1, "glm::mat2",    Reference, "mat2" },
	{ Variable::MAT3,            1, "glm::mat3",    Reference, "mat3" },
	{ Variable::MAT4,            1, "glm::mat4",    Reference, "mat4" },
	{ Variable::MAT3X4,          1, "glm::mat3x4",  Reference, "mat3x4" },
	{ Variable::MAT4X3,          1, "glm::mat4x3",  Reference, "mat4x3" },
	{ Variable::IMAGE2D,         1, "video::TextureUnit", Value,      "image2D" },
	{ Variable::SAMPLER1D,       1, "video::TextureUnit", Value,      "sampler1D" },
	{ Variable::SAMPLER2D,       1, "video::TextureUnit", Value,      "sampler2D" },
	{ Variable::SAMPLER2DARRAY,  1, "video::TextureUnit", Value,      "sampler2DArray" },
	{ Variable::SAMPLER2DARRAYSHADOW, 1, "video::TextureUnit", Value, "sampler2DArrayShadow" },
	{ Variable::SAMPLER3D,       1, "video::TextureUnit", Value,      "sampler3D" },
	{ Variable::SAMPLERCUBEMAP,  1, "video::TextureUnit", Value,      "samplerCube" },
	{ Variable::SAMPLER1DSHADOW, 1, "video::TextureUnit", Value,      "sampler1DShadow" },
	{ Variable::SAMPLER2DSHADOW, 1, "video::TextureUnit", Value,      "sampler2DShadow" }
};
static_assert(Variable::MAX == lengthof(cTypes), "mismatch in glsl types");

int getComponents(const Variable::Type type) {
	return resolveTypes(type).components;
}

Variable::Type getType(const std::string& type, int line) {
	int max = std::enum_value(Variable::MAX);
	for (int i = 0; i < max; ++i) {
		if (type == cTypes[i].glsltype) {
			return cTypes[i].type;
		}
	}
	Log::error("Unknown type given: %s at line %i - assuming float", type.c_str(), line);
	core_assert_msg(false, "Unknown type given: %s at line %i - assuming float", type.c_str(), line);
	return Variable::FLOAT;
}

static const ImageFormatType cImageFormat[] = {
	{video::ImageFormat::RGBA32F,               "rgba32f",         "GL_RGBA32F" },
	{video::ImageFormat::RGBA16F,               "rgba16f",         "GL_RGBA16F" },
	{video::ImageFormat::RG32F,                 "rg32f",           "GL_RG32F" },
	{video::ImageFormat::RG16F,                 "rg16f",           "GL_RG16F" },
	{video::ImageFormat::R11F_G11F_B10F,        "r11f_g11f_b10f",  "GL_R11F_G11F_B10F" },
	{video::ImageFormat::R32F,                  "r32f",            "GL_R32F" },
	{video::ImageFormat::R16F,                  "r16f",            "GL_R16F" },
	{video::ImageFormat::RGBA16,                "rgba16",          "GL_RGBA16" },
	{video::ImageFormat::RGB10_A2,              "rgb10_a2",        "GL_RGB10_A2" },
	{video::ImageFormat::RGBA8,                 "rgba8",           "GL_RGBA8" },
	{video::ImageFormat::RG16,                  "rg16",            "GL_RG16" },
	{video::ImageFormat::RG8,                   "rg8",             "GL_RG8" },
	{video::ImageFormat::R16,                   "r16",             "GL_R16" },
	{video::ImageFormat::R8,                    "r8",              "GL_R8" },
	{video::ImageFormat::RGBA16_SNORM,          "rgba16_snorm",    "GL_RGBA16_SNORM" },
	{video::ImageFormat::RGBA8_SNORM,           "rgba8_snorm",     "GL_RGBA8_SNORM" },
	{video::ImageFormat::RG16_SNORM,            "rg16_snorm",      "GL_RG16_SNORM" },
	{video::ImageFormat::RG8_SNORM,             "rg8_snorm",       "GL_RG8_SNORM" },
	{video::ImageFormat::R16_SNORM,             "r16_snorm",       "GL_R16_SNORM" },
	{video::ImageFormat::R8_SNORM,              "r8_snorm",        "GL_R8_SNORM" },
	{video::ImageFormat::RGBA32I,               "rgba32i",         "GL_RGBA32I" },
	{video::ImageFormat::RGBA16I,               "rgba16i",         "GL_RGBA16I" },
	{video::ImageFormat::RGBA8I,                "rgba8i",          "GL_RGBA8I" },
	{video::ImageFormat::RG32I,                 "rg32i",           "GL_RG32I" },
	{video::ImageFormat::RG16I,                 "rg16i",           "GL_RG16I" },
	{video::ImageFormat::RG8I,                  "rg8i",            "GL_RG8I" },
	{video::ImageFormat::R32I,                  "r32i",            "GL_R32I" },
	{video::ImageFormat::R16I,                  "r16i",            "GL_R16I" },
	{video::ImageFormat::R8I,                   "r8i",             "GL_R8I" },
	{video::ImageFormat::RGBA32UI,              "rgba32ui",        "GL_RGBA32UI" },
	{video::ImageFormat::RGBA16UI,              "rgba16ui",        "GL_RGBA16UI" },
	{video::ImageFormat::RGB10_A2UI,            "rgb10_a2ui",      "GL_RGB10_A2UI" },
	{video::ImageFormat::RGBA8UI,               "rgba8ui",         "GL_RGBA8UI" },
	{video::ImageFormat::RG32UI,                "rg32ui",          "GL_RG32UI" },
	{video::ImageFormat::RG16UI,                "rg16ui",          "GL_RG16UI" },
	{video::ImageFormat::RG8UI,                 "rg8ui",           "GL_RG8UI" },
	{video::ImageFormat::R32UI,                 "r32ui",           "GL_R32UI" },
	{video::ImageFormat::R16UI,                 "r16ui",           "GL_R16UI" },
	{video::ImageFormat::R8UI,                  "r8ui",            "GL_R8UI" }
};
static_assert((size_t)video::ImageFormat::Max == lengthof(cImageFormat), "mismatch in image formats");

video::ImageFormat getImageFormat(const std::string& glslType, int line) {
	const int max = std::enum_value(video::ImageFormat::Max);
	for (int i = 0; i < max; ++i) {
		if (glslType == cImageFormat[i].glsltype) {
			return cImageFormat[i].type;
		}
	}
	return video::ImageFormat::Max;
}

const char* getImageFormatGLType(video::ImageFormat format) {
	const int max = std::enum_value(video::ImageFormat::Max);
	for (int i = 0; i < max; ++i) {
		if (format == cImageFormat[i].type) {
			return cImageFormat[i].ctype;
		}
	}
	return nullptr;
}

std::string uniformSetterPostfix(const Variable::Type type, int amount) {
	switch (type) {
	case Variable::MAX:
		return "";
	case Variable::FLOAT:
		if (amount > 1) {
			return "1fv";
		}
		return "f";
	case Variable::DOUBLE:
		if (amount > 1) {
			return "1dv";
		}
		return "d";
	case Variable::UNSIGNED_INT:
		if (amount > 1) {
			return "1uiv";
		}
		return "ui";
	case Variable::BOOL:
	case Variable::INT:
		if (amount > 1) {
			return "1iv";
		}
		return "i";
	case Variable::DVEC2:
	case Variable::BVEC2:
	case Variable::UVEC2:
	case Variable::VEC2:
		if (amount > 1) {
			return "Vec2v";
		}
		return "Vec2";
	case Variable::DVEC3:
	case Variable::BVEC3:
	case Variable::UVEC3:
	case Variable::VEC3:
		if (amount > 1) {
			return "Vec3v";
		}
		return "Vec3";
	case Variable::DVEC4:
	case Variable::BVEC4:
	case Variable::UVEC4:
	case Variable::VEC4:
		if (amount > 1) {
			return "Vec4v";
		}
		return "Vec4";
	case Variable::IVEC2:
		if (amount > 1) {
			return "Vec2v";
		}
		return "Vec2";
	case Variable::IVEC3:
		if (amount > 1) {
			return "Vec3v";
		}
		return "Vec3";
	case Variable::IVEC4:
		if (amount > 1) {
			return "Vec4v";
		}
		return "Vec4";
	case Variable::MAT3X4:
	case Variable::MAT4X3:
	case Variable::MAT2:
	case Variable::MAT3:
	case Variable::MAT4:
		if (amount > 1) {
			return "Matrixv";
		}
		return "Matrix";
	case Variable::IMAGE2D:
	case Variable::SAMPLER1D:
	case Variable::SAMPLER2D:
	case Variable::SAMPLER3D:
	case Variable::SAMPLER1DSHADOW:
	case Variable::SAMPLER2DSHADOW:
	case Variable::SAMPLER2DARRAY:
	case Variable::SAMPLER2DARRAYSHADOW:
		if (amount > 1) {
			// https://www.opengl.org/wiki/Data_Type_%28GLSL%29#Opaque_arrays
			if (video::Shader::glslVersion < video::GLSLVersion::V400) {
				Log::warn("Sampler arrays are only allowed under special circumstances - don't do this for GLSL < 4.0");
			}
			// TODO: doesn't work yet, video::TextureUnit support is needed here
			return "1iv";
		}
		return "";
	case Variable::SAMPLERCUBEMAP:
		if (amount > 1) {
			return "1iv";
		}
		return "i";
	}
	return "";
}

std::string convertName(const std::string& in, bool firstUpper) {
	std::string out;
	std::vector<std::string> nameParts;
	core::string::splitString(in, nameParts, "_");
	for (std::string& n : nameParts) {
		if (n.length() > 1 || nameParts.size() < 2) {
			if (!firstUpper) {
				firstUpper = true;
			} else {
				n[0] = SDL_toupper(n[0]);
			}
			out += n;
		}
	}
	if (out.empty()) {
		out = in;
	}
	return out;
}


#define USE_ALIGN_AS 1

/**
 * The size of each element in the array will be the size of the element type, rounded up to a multiple of the
 * size of a vec4. This is also the array’s alignment. The array’s size will be this rounded-up element’s size
 * times the number of elements in the array.
 * If the member is a three-component vector with components consuming N basic machine units, the base alignment is 4N.
 *
 * @note:
 * a float needs 4 bytes and it's 4 bytes aligned
 * a vec3 needs 12 bytes and it's 16 bytes aligned
 * a vec4 needs 16 bytes and it's 16 bytes aligned
 */
std::string std140Align(const Variable& v) {
#if USE_ALIGN_AS > 0
	// TODO: generate uniform buffer struct - enforce std140 layout
	// TODO: extract uniform blocks into aligned structs and generate methods to update them
	//       align them via GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT - use glBindBufferRange
	//       GL_MAX_UNIFORM_BLOCK_SIZE
	const Types& cType = resolveTypes(v.type);
	if (cType.type == Variable::Type::VEC2 || cType.type == Variable::Type::VEC3 || cType.type == Variable::Type::VEC4
	 || cType.type == Variable::Type::DVEC2 || cType.type == Variable::Type::DVEC3 || cType.type == Variable::Type::DVEC4
	 || cType.type == Variable::Type::IVEC2 || cType.type == Variable::Type::IVEC3 || cType.type == Variable::Type::IVEC4
	 || cType.type == Variable::Type::BVEC2 || cType.type == Variable::Type::BVEC3 || cType.type == Variable::Type::BVEC4) {
		return "alignas(16) ";
	}
	if (cType.type == Variable::Type::FLOAT || cType.type == Variable::Type::DOUBLE) {
		return "alignas(4) ";
	}
#endif
	return "";
}

std::string std140Padding(const Variable& v, int& padding) {
#if USE_ALIGN_AS == 0
	const Types& cType = cTypes[v.type];
	if (cType.type == Variable::Type::VEC3
	 || cType.type == Variable::Type::DVEC3
	 || cType.type == Variable::Type::IVEC3
	 || cType.type == Variable::Type::BVEC3) {
		return "\t\tfloat _padding" + std::to_string(padding++) + ";\n";
	}
#endif
	return "";
}

size_t std140Size(const Variable& v) {
	const Types& cType = resolveTypes(v.type);
	int components = cType.components;
	int bytes = 4;
	if (cType.type == Variable::Type::DVEC2
	 || cType.type == Variable::Type::DVEC3
	 || cType.type == Variable::Type::DVEC4
	 || cType.type == Variable::Type::DOUBLE) {
		bytes = 8;
	}
	if (cType.type == Variable::Type::VEC2
	 || cType.type == Variable::Type::DVEC2
	 || cType.type == Variable::Type::IVEC2
	 || cType.type == Variable::Type::BVEC2) {
		components = 2;
	}
	if (cType.type == Variable::Type::VEC3
	 || cType.type == Variable::Type::DVEC3
	 || cType.type == Variable::Type::IVEC3
	 || cType.type == Variable::Type::BVEC3) {
		components = 4;
	}
	if (cType.type == Variable::Type::MAT2) {
		components = 4;
	} else if (cType.type == Variable::Type::MAT3) {
		components = 9; // FIXME
	} else if (cType.type == Variable::Type::MAT4) {
		components = 16;
	} else if (cType.type == Variable::Type::MAT3X4) {
		components = 16; // FIXME
	} else if (cType.type == Variable::Type::MAT4X3) {
		components = 16; // FIXME
	}
	if (v.arraySize > 0) {
		return components * bytes * v.arraySize;
	}
	return components * bytes;
}

std::string std430Align(const Variable& v) {
	// TODO: check this layout
	return std140Align(v);
}

size_t std430Size(const Variable& v) {
	// TODO: check this layout
	return std140Size(v);
}

std::string std430Padding(const Variable& v, int& padding) {
	// TODO: check this layout
	return std140Padding(v, padding);
}

const Types& resolveTypes(Variable::Type type) {
	int max = std::enum_value(Variable::MAX);
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
