/**
 * @file
 */
#pragma once

#include "compute/Types.h"
#include <string>

namespace util {

extern bool isQualifier(const std::string& token);

extern std::string convert(const std::string& type);

extern std::string convertType(const std::string& type, std::string& arrayDefinition, int *arraySize = nullptr);

extern std::string vectorType(const std::string& type);

extern std::string toString(compute::BufferFlag flagMask);

}
