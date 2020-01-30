/**
 * @file
 */

#pragma once

#include <stdint.h>
#include "core/String.h"
#include <vector>
#include <set>
#include <unordered_map>

namespace persistence {

struct Constraint {
	std::vector<core::String> fields;
	// bitmask from persistence::Model::ConstraintType
	uint32_t types;
};
struct ForeignKey {
	core::String table;
	core::String field;
};

typedef std::unordered_map<core::String, Constraint> Constraints;
typedef std::unordered_map<core::String, ForeignKey> ForeignKeys;
typedef std::vector<std::set<core::String>> UniqueKeys;
typedef std::vector<core::String> PrimaryKeys;

}
