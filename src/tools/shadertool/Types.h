/**
 * @file
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <glm/vec3.hpp>

enum PassBy {
	Value,
	Reference,
	Pointer
};

enum class BlockLayout {
	unknown,
	std140,
	std430
};

enum class PrimitiveType {
	None,
	Points,
	Lines,
	LinesAdjacency,
	Triangles,
	TrianglesAdjacency,
	LineStrip,
	TriangleStrip,
	Max
};

struct Variable {
	enum Type {
		DOUBLE = 0, FLOAT, UNSIGNED_INT, INT, BOOL,
		DVEC2, DVEC3, DVEC4, BVEC2, BVEC3, BVEC4,
		UVEC2, UVEC3, UVEC4, IVEC2, IVEC3, IVEC4,
		VEC2, VEC3, VEC4,
		MAT2, MAT3, MAT4, MAT3X4, MAT4X3,
		SAMPLER1D, SAMPLER2D, SAMPLER3D,
		SAMPLERCUBEMAP, SAMPLER2DARRAYSHADOW, SAMPLER2DARRAY,
		SAMPLER1DSHADOW, SAMPLER2DSHADOW,
		// compute shaders
		IMAGE2D,
		MAX
		// TODO: atomics
	};
	Type type;
	std::string name;
	int arraySize = 0;

	inline bool isSingleInteger() const {
		return isSampler() || type == Variable::INT || type == Variable::UNSIGNED_INT;
	}

	inline bool isSampler() const {
		return type == Variable::SAMPLER1D || type == Variable::SAMPLER2D || type == Variable::SAMPLER3D
		 || type == Variable::SAMPLER2DSHADOW || type == Variable::SAMPLER1DSHADOW || type == Variable::SAMPLERCUBEMAP
		 || type == Variable::IMAGE2D;
	}

	inline bool isInteger() const {
		return type == Variable::UNSIGNED_INT || type == Variable::INT || type == Variable::IVEC2 || type == Variable::IVEC3 || type == Variable::IVEC4;
	}
};

enum class LayoutImageFormat {
	// floating point
	RGBA32F,
	RGBA16F,
	RG32F,
	RG16F,
	R11F_G11F_B10F,
	R32F,
	R16F,
	RGBA16,
	RGB10_A2,
	RGBA8,
	RG16,
	RG8,
	R16,
	R8,
	RGBA16_SNORM,
	RGBA8_SNORM,
	RG16_SNORM,
	RG8_SNORM,
	R16_SNORM,
	R8_SNORM,

	// signed integer
	RGBA32I,
	RGBA16I,
	RGBA8I,
	RG32I,
	RG16I,
	RG8I,
	R32I,
	R16I,
	R8I,

	// unsigned integer
	RGBA32UI,
	RGBA16UI,
	RGB10_A2UI,
	RGBA8UI,
	RG32UI,
	RG16UI,
	RG8UI,
	R32UI,
	R16UI,
	R8UI,

	Max
};

struct Types {
	Variable::Type type;
	int components;
	const char* ctype;
	PassBy passBy;
	const char* glsltype;
};

// https://www.khronos.org/opengl/wiki/Layout_Qualifier_(GLSL)
struct Layout {
	int binding = -1;
	int components = -1;
	int offset = -1;
	int index = -1;
	int location = -1;
	int transformFeedbackOffset = -1; // 4.4
	int transformFeedbackBuffer = -1; // 4.4
	int tesselationVertices = -1; // 4.0
	int maxGeometryVertices = -1; // 4.0
	bool originUpperLeft = false; // 4.0
	bool pixelCenterInteger = false; // 4.0
	bool earlyFragmentTests = false; // 4.2
	glm::ivec3 localSize { -1 };
	PrimitiveType primitiveType = PrimitiveType::None;
	BlockLayout blockLayout = BlockLayout::unknown;
	LayoutImageFormat imageFormat = LayoutImageFormat::Max;

	std::string typeAlign(const Variable& v) const;

	size_t typeSize(const Variable& v) const;

	std::string typePadding(const Variable& v, int& padding) const;
};

struct LayoutImageFormatType {
	LayoutImageFormat type;
	const char* glsltype;
	const char* ctype;
};

struct UniformBlock {
	std::string name;
	std::vector<Variable> members;
	Layout layout;
};

struct ShaderStruct {
	std::string name;
	std::string filename;
	// both
	std::vector<Variable> uniforms;
	std::unordered_map<std::string, Layout> layouts;
	std::vector<UniformBlock> uniformBlocks;
	// vertex only
	std::vector<Variable> attributes;
	// vertex only
	std::vector<Variable> varyings;
	// fragment only
	std::vector<Variable> outs;
};
