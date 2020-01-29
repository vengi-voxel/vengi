/**
 * @file
 */
#pragma once

#include "compute/Types.h"
#include "core/String.h"

namespace computeshadertool {
namespace util {

struct CLTypeMapping {
	std::string type {};
	int arraySize = 0;
};

extern bool isQualifier(const std::string& token);

extern CLTypeMapping vectorType(const std::string& type);

extern int alignment(const std::string& type);

/**
 * @brief convert the given input string into lower- or upper-camel-case
 * @param in The string to convert
 * @param firstUpper Convert to upper camel case
 * @return The camel case string
 */
extern std::string convertName(const std::string& in, bool firstUpper);

extern std::string toString(compute::BufferFlag flagMask);

}
}
