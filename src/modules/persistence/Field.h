/**
 * @file
 */

#pragma once

#include <stdint.h>
#include "core/String.h"
#include "FieldType.h"
#include "ConstraintType.h"

namespace persistence {

// don't change the order without changing the string mapping
enum class Operator {
	ADD,
	SUBTRACT,
	SET,

	MAX
};

struct Field {
	core::String name;
	FieldType type = FieldType::STRING;
	Operator updateOperator = Operator::SET;
	// bitmask from ConstraintType
	uint32_t contraintMask = 0u;
	core::String defaultVal = "";
	int length = 0;
	intptr_t offset = -1;
	intptr_t nulloffset = -1;
	// a value that is used to decide whether the field
	// has a valid value set (which might also be null)
	intptr_t validoffset = -1;

	inline bool isAutoincrement() const {
		return (contraintMask & core::enumVal(ConstraintType::AUTOINCREMENT)) != 0u;
	}

	inline bool isIndex() const {
		return (contraintMask & core::enumVal(ConstraintType::INDEX)) != 0u;
	}

	inline bool isNotNull() const {
		return (contraintMask & core::enumVal(ConstraintType::NOTNULL)) != 0u;
	}

	inline bool isPrimaryKey() const {
		return (contraintMask & core::enumVal(ConstraintType::PRIMARYKEY)) != 0u;
	}

	inline bool isLower() const {
		return (contraintMask & core::enumVal(ConstraintType::LOWERCASE)) != 0u;
	}

	inline bool isUnique() const {
		return (contraintMask & core::enumVal(ConstraintType::UNIQUE)) != 0u;
	}

	inline bool isForeignKey() const {
		return (contraintMask & core::enumVal(ConstraintType::FOREIGNKEY)) != 0u;
	}
};

}
