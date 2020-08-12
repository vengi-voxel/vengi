/**
 * @file
 */

#include "Util.h"
#include "core/StringUtil.h"
#include "compute/Types.h"
#include "core/Common.h"

namespace computeshadertool {
namespace util {

static const struct TypeMapping {
	const char* computeType;
	const core::String ctype;
} Types[] = {
	{"char",      "int8_t"},
	{"uchar",     "uint8_t"},
	{"short",     "int16_t"},
	{"ushort",    "uint16_t"},
	{"int",       "int32_t"},
	{"uint",      "uint32_t"},
	{"long",      "int64_t"},
	{"ulong",     "uint64_t"},
	{"float",     "float"},
	{"double",    "double"},
	{"half",      "uint16_t"},
	{"image3d_t", "Texture"},
	{"image2d_t", "Texture"},
	{nullptr,     ""}
};

static const struct VecMapping {
	const char* computeType;
	const core::String ctype;
} Vecs[] = {
	{"float2",   "glm::vec2"},
	{"float3",   "glm::vec3"},
	{"float4",   "glm::vec4"},
	{"int2",     "glm::ivec2"},
	{"int3",     "glm::ivec3"},
	{"int4",     "glm::ivec4"},
	{"uint2",    "glm::uvec2"},
	{"uint3",    "glm::uvec3"},
	{"uint4",    "glm::uvec4"},
	{"double2",  "glm::dvec2"},
	{"double3",  "glm::dvec3"},
	{"double4",  "glm::dvec4"},
	{nullptr,    ""}
};

static const struct TypeAlignment {
	const char* type;
	const int alignment;
} Alignments[] = {
	{"int16_t",     2},
	{"uint16_t",    2},
	{"int32_t",     4},
	{"uint32_t",    4},
	{"int64_t",     8},
	{"uint64_t",    8},
	{"float",       4},
	{"double",      8},
	{"glm::vec2",   8},
	{"glm::vec3",  16},
	{"glm::vec4",  16},
	{"glm::ivec2",  8},
	{"glm::ivec3", 16},
	{"glm::ivec4", 16},
	{"glm::uvec2",  8},
	{"glm::uvec3", 16},
	{"glm::uvec4", 16},
	{"glm::dvec2", 16},
	{"glm::dvec3", 32},
	{"glm::dvec4", 32},
	{nullptr,       0}
};

bool isQualifier(const core::String& token) {
	return token == "const" || core::string::startsWith(token, "__");
}

static int arraySizeFromType(const core::String& type) {
	if (type.empty()) {
		return 0u;
	}
	const size_t size = type.size();
	for (size_t i = 1; i < size; ++i) {
		const char c = type[size - i];
		if (c == '*') {
			continue;
		}
		if (c == ' ') {
			continue;
		}
		if (c >= '0' && c <= '9') {
			return c - '0';
		}
		break;
	}
	return 0u;
}

static CLTypeMapping convert(const core::String& type) {
	if (type.empty()) {
		static const CLTypeMapping mapping = CLTypeMapping();
		return mapping;
	}
	const bool ispointer = type[type.size() - 1] == '*';

	CLTypeMapping mapping;
	for (const TypeMapping* t = Types; t->computeType != nullptr; ++t) {
		if (!core::string::startsWith(type, t->computeType)) {
			continue;
		}
		if (ispointer) {
			//return t->ctype + " *";
		}
		mapping.type = t->ctype;
		mapping.arraySize = arraySizeFromType(type);
		return mapping;
	}
	mapping.type = type;
	mapping.arraySize = arraySizeFromType(type);
	return mapping;
}

int alignment(const core::String& type) {
	for (const TypeAlignment* t = Alignments; t->type != nullptr; ++t) {
		if (type == t->type) {
			return t->alignment;
		}
	}

	return 1;
}

CLTypeMapping vectorType(const core::String& type) {
	if (type.empty()) {
		static const CLTypeMapping mapping = CLTypeMapping();
		return mapping;
	}
	for (const VecMapping* t = Vecs; t->computeType != nullptr; ++t) {
		if (!core::string::startsWith(type, t->computeType)) {
			continue;
		}
		CLTypeMapping mapping;
		mapping.type = t->ctype;
		return mapping;
	}
	return convert(type);
}

core::String toString(compute::BufferFlag flagMask) {
	core::String str;

#define CHECK_MASK(mask) \
	if ((flagMask & compute::BufferFlag::mask) == compute::BufferFlag::mask) { \
		if (!str.empty()) \
			str += " | "; \
		str += "compute::BufferFlag::" CORE_STRINGIFY(mask); \
	}
	CHECK_MASK(ReadWrite)
	CHECK_MASK(WriteOnly)
	CHECK_MASK(ReadOnly)
	CHECK_MASK(UseHostPointer)
	CHECK_MASK(AllocHostPointer)
	CHECK_MASK(CopyHostPointer)
#undef CHECK_MASK
	if (str.empty()) {
		str += "compute::BufferFlag::None";
	}
	return str;
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

}
}
