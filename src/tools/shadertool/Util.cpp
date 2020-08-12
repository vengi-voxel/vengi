/**
 * @file
 */

#include "Util.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/Assert.h"
#include "core/ArrayLength.h"
#include <vector>
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
	{ Variable::SAMPLER2DMS,     1, "video::TextureUnit", Value,      "sampler2DMS" },
	{ Variable::SAMPLERCUBEMAP,  1, "video::TextureUnit", Value,      "samplerCube" },
	{ Variable::SAMPLER1DSHADOW, 1, "video::TextureUnit", Value,      "sampler1DShadow" },
	{ Variable::SAMPLER2DSHADOW, 1, "video::TextureUnit", Value,      "sampler2DShadow" }
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
	case Variable::SAMPLER2DMS:
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
		return "";
	}
	return "";
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
core::String std140Align(const Variable& v) {
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

core::String std140Padding(const Variable& v, int& padding) {
#if USE_ALIGN_AS == 0
	const Types& cType = cTypes[v.type];
	if (cType.type == Variable::Type::VEC3
	 || cType.type == Variable::Type::DVEC3
	 || cType.type == Variable::Type::IVEC3
	 || cType.type == Variable::Type::BVEC3) {
		return "\t\tfloat _padding" + core::string::toString(padding++) + ";\n";
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

core::String std430Align(const Variable& v) {
	// TODO: check this layout
	return std140Align(v);
}

size_t std430Size(const Variable& v) {
	// TODO: check this layout
	return std140Size(v);
}

core::String std430Padding(const Variable& v, int& padding) {
	// TODO: check this layout
	return std140Padding(v, padding);
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
