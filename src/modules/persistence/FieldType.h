/**
 * @file
 */

#pragma once

#include "core/Enum.h"
#include <string>

namespace persistence {

// don't change the order without changing the string mapping
enum class FieldType {
	STRING,
	TEXT,
	LONG,
	INT,
	PASSWORD,
	TIMESTAMP,
	BOOLEAN,
	SHORT,
	BYTE,
	DOUBLE,
	BLOB,
	MAX
};
static constexpr int MAX_FIELDTYPES = std::enum_value(FieldType::MAX);

extern FieldType toFieldType(const std::string& type);
extern const char* toFieldType(const FieldType type);

}
