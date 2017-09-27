/**
 * @file
 */

#pragma once

namespace persistence {

// don't change the order - code generator relies on this
enum class ConstraintType {
	UNIQUE = 1 << 0,
	PRIMARYKEY = 1 << 1,
	AUTOINCREMENT = 1 << 2,
	NOTNULL = 1 << 3
};
static constexpr int MAX_CONSTRAINTTYPES = 4;

}
