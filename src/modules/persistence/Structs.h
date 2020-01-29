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
	std::vector<std::string> fields;
	// bitmask from persistence::Model::ConstraintType
	uint32_t types;
};
struct ForeignKey {
	core::String table;
	core::String field;
};

typedef std::unordered_map<std::string, Constraint> Constraints;
typedef std::unordered_map<std::string, ForeignKey> ForeignKeys;
typedef std::vector<std::set<std::string>> UniqueKeys;
typedef std::vector<std::string> PrimaryKeys;

}
