#pragma once

#include "core/Common.h"
#include "core/Array.h"
#include "persistence/Model.h"

static const char *FieldTypeNames[] = {
	CORE_STRINGIFY(STRING),
	CORE_STRINGIFY(TEXT),
	CORE_STRINGIFY(LONG),
	CORE_STRINGIFY(INT),
	CORE_STRINGIFY(PASSWORD),
	CORE_STRINGIFY(TIMESTAMP),
	CORE_STRINGIFY(BOOLEAN),
	CORE_STRINGIFY(SHORT)
};
static_assert(lengthof(FieldTypeNames) == persistence::MAX_FIELDTYPES, "Invalid field type mapping");

static const char *ConstraintTypeNames[] = {
	CORE_STRINGIFY(UNIQUE),
	CORE_STRINGIFY(PRIMARYKEY),
	CORE_STRINGIFY(AUTOINCREMENT),
	CORE_STRINGIFY(NOTNULL)
};
static_assert(lengthof(ConstraintTypeNames) == persistence::MAX_CONSTRAINTTYPES, "Invalid constraint type mapping");

static const char *OperatorNames[] = {
	CORE_STRINGIFY(ADD),
	CORE_STRINGIFY(SUBTRACT),
	CORE_STRINGIFY(SET)
};
static_assert(lengthof(OperatorNames) == (int)persistence::Operator::MAX, "Invalid operator mapping");
