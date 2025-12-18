/**
 * @file
 */

#pragma once

#include "video/Types.h"
#include "core/String.h"
#include "core/collection/DynamicStringMap.h"
#include "core/collection/List.h"
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>

enum PassBy {
	Value,
	Reference,
	Pointer
};

enum class BlockLayout {
	unknown,
	std140
};

struct Variable {
	enum Type {
		FLOAT, UNSIGNED_INT, INT,
		UVEC2, UVEC3, UVEC4, IVEC2, IVEC3, IVEC4,
		VEC2, VEC3, VEC4, MAT4,
		SAMPLER1D, SAMPLER2D, SAMPLER3D, SAMPLER2DMS,
		SAMPLERCUBEMAP, SAMPLER2DARRAYSHADOW, SAMPLER2DARRAY,
		SAMPLER1DSHADOW, SAMPLER2DSHADOW, USAMPLER3D, IMAGE2D,
		MAX
		// TODO: atomics
	};
	Type type = Type::MAX;
	core::String name;
	int arraySize = 0;

	inline const char* dataType() const {
		switch (type) {
		case VEC2:
		case VEC3:
		case VEC4:
		case MAT4:
		case FLOAT:
			return "video::DataType::Float";
		case UVEC2:
		case UVEC3:
		case UVEC4:
		case UNSIGNED_INT:
			return "video::DataType::UnsignedInt";
		case IVEC2:
		case IVEC3:
		case IVEC4:
		case INT:
			return "video::DataType::Int";
		case SAMPLER1D:
		case SAMPLER2D:
		case SAMPLER3D:
		case SAMPLERCUBEMAP:
		case SAMPLER2DARRAYSHADOW:
		case SAMPLER2DMS:
		case SAMPLER2DARRAY:
		case SAMPLER1DSHADOW:
		case SAMPLER2DSHADOW:
		case USAMPLER3D:
		case IMAGE2D:
			return "video::DataType::Int";
		case MAX:
			break;
		}
		return "video::DataType::Max";
	}

	inline bool isSingleInteger() const {
		return isSampler() || isImage() || type == Variable::INT || type == Variable::UNSIGNED_INT;
	}

	inline bool isSampler() const {
		return type == Variable::SAMPLER1D || type == Variable::SAMPLER2D || type == Variable::SAMPLER3D || type == Variable::SAMPLER2DMS
		 || type == Variable::SAMPLER2DSHADOW || type == Variable::SAMPLER1DSHADOW || type == Variable::SAMPLERCUBEMAP || type == Variable::USAMPLER3D;
	}

	inline bool isImage() const {
		return type == Variable::IMAGE2D;
	}

	inline bool isInteger() const {
		return type == Variable::UNSIGNED_INT || type == Variable::INT || type == Variable::IVEC2 || type == Variable::IVEC3 || type == Variable::IVEC4;
	}
};

struct Types {
	Variable::Type type;
	int size;
	int align;
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
	video::Primitive primitiveType = video::Primitive::Max;
	BlockLayout blockLayout = BlockLayout::unknown;
	video::ImageFormat imageFormat = video::ImageFormat::Max;

	size_t typeSize(const Variable& v) const;
	int typeAlign(const Variable& v) const;
};

struct ImageFormatType {
	video::ImageFormat type;
	const char* glsltype;
	const char* ctype;
};

struct PrimitiveType {
	video::Primitive type;
	const char* str;
};

struct BufferBlock {
	core::String name;
	core::List<Variable> members;
	Layout layout;
};

struct InOut {
	Layout layout;
};

struct ShaderStruct {
	core::String name;
	core::String filename;
	// both
	core::List<Variable> uniforms;
	core::DynamicStringMap<Layout> layouts;
	core::DynamicStringMap<core::String> constants;
	// ubo
	// https://github.com/freedesktop-unofficial-mirror/piglit-test/blob/26323b93557675aa9a1e9675c6eddbc92a69ccda/tests/spec/arb_uniform_buffer_object/uniform-types.c
	core::List<BufferBlock> uniformBlocks;
	// ssbo
	core::List<BufferBlock> bufferBlocks;
	// vertex only
	core::List<Variable> attributes;
	// vertex only
	core::List<Variable> varyings;
	// fragment only
	core::List<Variable> outs;
	InOut in;
	InOut out;
};
