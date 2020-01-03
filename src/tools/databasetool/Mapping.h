#pragma once

#include "core/Common.h"
#include "core/ArrayLength.h"
#include "persistence/FieldType.h"

static const char *ConstraintTypeNames[] = {
	CORE_STRINGIFY(UNIQUE),
	CORE_STRINGIFY(PRIMARYKEY),
	CORE_STRINGIFY(AUTOINCREMENT),
	CORE_STRINGIFY(NOTNULL),
	CORE_STRINGIFY(INDEX),
	CORE_STRINGIFY(FOREIGNKEY),
	CORE_STRINGIFY(LOWERCASE)
};
static_assert(lengthof(ConstraintTypeNames) == persistence::MAX_CONSTRAINTTYPES, "Invalid constraint type mapping");

static const char *OperatorNames[] = {
	CORE_STRINGIFY(ADD),
	CORE_STRINGIFY(SUBTRACT),
	CORE_STRINGIFY(SET)
};
static_assert(lengthof(OperatorNames) == (int)persistence::Operator::MAX, "Invalid operator mapping");
