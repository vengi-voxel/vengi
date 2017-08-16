/**
 * @file
 */
#include "Util.h"
#include "core/String.h"

namespace util {

static const struct TypeMapping {
	const char* computeType;
	const std::string ctype;
} Types[] = {
	{"char",     "int8_t"},
	{"uchar",    "uint8_t"},
	{"short",    "int16_t"},
	{"ushort",   "uint16_t"},
	{"int",      "int32_t"},
	{"uint",     "uint32_t"},
	{"long",     "int64_t"},
	{"ulong",    "uint64_t"},
	{"float",    "float"},
	{"double",   "double"},
	{"half",     "uint16_t"},
	{nullptr,    ""}
};

static const struct VecMapping {
	const char* computeType;
	const std::string ctype;
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

bool isQualifier(const std::string& token) {
	return token == "const" || core::string::startsWith(token, "__");
}

std::string convert(const std::string& type) {
	if (type.empty()) {
		return type;
	}
	const bool ispointer = type[type.size() - 1] == '*';

	for (const TypeMapping* t = Types; t->computeType != nullptr; ++t) {
		if (!core::string::startsWith(type, t->computeType)) {
			continue;
		}
		if (ispointer) {
			//return t->ctype + " *";
		}
		return t->ctype;
	}
	return type;
}

std::string vectorType(const std::string& type) {
	if (type.empty()) {
		return type;
	}
	for (const VecMapping* t = Vecs; t->computeType != nullptr; ++t) {
		if (!core::string::startsWith(type, t->computeType)) {
			continue;
		}
		return t->ctype;
	}
	return convert(type);
}

std::string convertType(const std::string& type, std::string& arrayDefinition, int *arraySize) {
	char c = '\0';
	const size_t size = type.size();
	if (arraySize != nullptr) {
		*arraySize = 0;
	}
	size_t i;
	for (i = 1; i < size; ++i) {
		c = type[size - i];
		if (c == '*') {
			continue;
		}
		if (c == ' ') {
			continue;
		}
		break;
	}
	arrayDefinition = "";
	if (c < '0' || c > '9') {
		return convert(type);
	}
	for (; i < size; ++i) {
		c = type[size - i];
		if (c >= '0' && c <= '9') {
			const char buf[] = {c, '\0'};
			arrayDefinition.append(buf);
			continue;
		}
		break;
	}
	if (arraySize != nullptr) {
		*arraySize = core::string::toInt(arrayDefinition);
	}

	arrayDefinition = "[" + arrayDefinition + "]";
	const std::string& sub = type.substr(0, size - i);
	return convert(type);
}

std::string toString(compute::BufferFlag flagMask) {
	std::string str;

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

}
