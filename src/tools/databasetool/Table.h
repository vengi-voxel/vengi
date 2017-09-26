#pragma once

#include "persistence/Model.h"
#include <map>
#include <vector>
#include <cstdint>
#include <string>
#include <set>

namespace databasetool {

// TODO: sort for insertion order - keep it stable
typedef std::map<std::string, persistence::Model::Field> Fields;

struct Table {
	std::string name;
	std::string classname;
	std::string namespaceSrc = "backend";
	Fields fields;
	persistence::Model::Constraints constraints;
	int primaryKeys = 0;
	std::vector<std::set<std::string>> uniqueKeys;
};

}
