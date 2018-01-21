/**
 * @file
 */

#pragma once

#include <stdint.h>
#include <string>
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
	std::string table;
	std::string field;
};

typedef std::unordered_map<std::string, Constraint> Constraints;
typedef std::unordered_map<std::string, ForeignKey> ForeignKeys;
typedef std::vector<std::set<std::string>> UniqueKeys;
typedef std::vector<std::string> PrimaryKeys;

typedef Constraints* ConstraintsPtr;
typedef ForeignKeys* ForeignKeysPtr;
typedef UniqueKeys* UniqueKeysPtr;
typedef PrimaryKeys* PrimaryKeysPtr;

}
