/**
 * @file
 */

#include "core/Common.h"
#include "core/StringUtil.h"
#include "core/ArrayLength.h"
#include "FieldType.h"

namespace persistence {

static const char *FieldTypeNames[] = {
	CORE_STRINGIFY(STRING),
	CORE_STRINGIFY(TEXT),
	CORE_STRINGIFY(LONG),
	CORE_STRINGIFY(INT),
	CORE_STRINGIFY(PASSWORD),
	CORE_STRINGIFY(TIMESTAMP),
	CORE_STRINGIFY(BOOLEAN),
	CORE_STRINGIFY(SHORT),
	CORE_STRINGIFY(BYTE),
	CORE_STRINGIFY(DOUBLE),
	CORE_STRINGIFY(BLOB)
};
static_assert(lengthof(FieldTypeNames) == persistence::MAX_FIELDTYPES, "Invalid field type mapping");

FieldType toFieldType(const core::String& type) {
	for (int i = 0; i < persistence::MAX_FIELDTYPES; ++i) {
		if (core::string::iequals(type, FieldTypeNames[i])) {
			return (persistence::FieldType)i;
		}
	}
	return persistence::FieldType::MAX;
}

const char* toFieldType(const FieldType type) {
	return FieldTypeNames[(int)type];
}

}
