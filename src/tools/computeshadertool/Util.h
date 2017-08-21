/**
 * @file
 */
#pragma once

#include "compute/Types.h"
#include <string>

namespace util {

struct CLTypeMapping {
	std::string type {};
	int arraySize = 0;
};

extern bool isQualifier(const std::string& token);

extern CLTypeMapping vectorType(const std::string& type);

extern int alignment(const std::string& type);

extern std::string toString(compute::BufferFlag flagMask);

}
