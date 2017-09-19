#pragma once

#include "persistence/Model.h"
#include <map>
#include <vector>
#include <cstdint>
#include <string>

namespace databasetool {

struct Constraint {
	std::vector<std::string> fields;
	// bitmask from persistence::Model::ConstraintType
	uint32_t types;
};

typedef std::map<std::string, Constraint> Constraints;
// TODO: sort for insertion order - keep it stable
typedef std::map<std::string, persistence::Model::Field> Fields;

struct Table {
	std::string name;
	std::string classname;
	std::string namespaceSrc = "backend";
	Fields fields;
	Constraints contraints;
	int primaryKeys = 0;
	std::vector<std::string> uniqueKeys;
};

}
