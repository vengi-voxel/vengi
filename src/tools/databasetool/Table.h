#pragma once

#include "persistence/Model.h"
#include <cstdint>
#include <map>
#include <string>

namespace databasetool {

// TODO: sort for insertion order - keep it stable
typedef std::map<std::string, persistence::Field> Fields;

struct Table {
	std::string name;
	std::string classname;
	std::string namespaceSrc = "backend";
	std::string schema = "public";
	Fields fields;
	persistence::Constraints constraints;
	persistence::ForeignKeys foreignKeys;
	int primaryKeys = 0;
	persistence::UniqueKeys uniqueKeys;
	int autoIncrementStart = 1;
};

}
