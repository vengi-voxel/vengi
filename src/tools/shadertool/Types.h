/**
 * @file
 */

#pragma once

#include <string>
#include <vector>

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
		SAMPLER1DSHADOW, SAMPLER2DSHADOW, MAX
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
		 || type == Variable::SAMPLER2DSHADOW || type == Variable::SAMPLER1DSHADOW || type == Variable::SAMPLERCUBEMAP;
	}

	inline bool isInteger() const {
		return type == Variable::UNSIGNED_INT || type == Variable::INT || type == Variable::IVEC2 || type == Variable::IVEC3 || type == Variable::IVEC4;
	}
};

struct Types {
	Variable::Type type;
	int components;
	const char* ctype;
	PassBy passBy;
	const char* glsltype;
};

struct Layout {
	int binding = 0;
	int components = 0;
	int offset = 0;
	int index = 0;
	int location = -1;
	int transformFeedbackOffset = -1; // 4-4
	int transformFeedbackBuffer = -1; // 4-4
	int tesselationVertices = -1; // 4.0
	int maxGeometryVertices = -1; // 4.0
	bool originUpperLeft = false; // 4.0
	bool pixelCenterInteger = false; // 4.0
	bool earlyFragmentTests = false; // 4.2
	PrimitiveType primitiveType = PrimitiveType::None;
	BlockLayout blockLayout = BlockLayout::unknown;

	std::string typeAlign(const Variable& v) const;

	size_t typeSize(const Variable& v) const;

	std::string typePadding(const Variable& v, int& padding) const;
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
	std::vector<UniformBlock> uniformBlocks;
	// vertex only
	std::vector<Variable> attributes;
	// vertex only
	std::vector<Variable> varyings;
	// fragment only
	std::vector<Variable> outs;
};
