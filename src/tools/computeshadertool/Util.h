/**
 * @file
 */
#pragma once

#include "compute/Types.h"
#include "core/String.h"

namespace computeshadertool {
namespace util {

struct CLTypeMapping {
	core::String type {};
	int arraySize = 0;
};

extern bool isQualifier(const core::String& token);

extern CLTypeMapping vectorType(const core::String& type);

extern int alignment(const core::String& type);

/**
 * @brief convert the given input string into lower- or upper-camel-case
 * @param in The string to convert
 * @param firstUpper Convert to upper camel case
 * @return The camel case string
 */
extern core::String convertName(const core::String& in, bool firstUpper);

extern core::String toString(compute::BufferFlag flagMask);

}
}
