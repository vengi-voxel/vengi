#pragma once

#include "persistence/Field.h"
#include "persistence/Structs.h"
#include <stdint.h>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include "core/String.h"

namespace databasetool {

// TODO: sort for insertion order - keep it stable
typedef std::map<std::string, persistence::Field> Fields;

struct Table {
	core::String name;
	core::String classname;
	core::String namespaceSrc = "backend";
	core::String schema = "public";
	Fields fields;
	persistence::Constraints constraints;
	persistence::ForeignKeys foreignKeys;
	int primaryKeys = 0;
	persistence::UniqueKeys uniqueKeys;
	core::String autoIncrementField;
	int autoIncrementStart = 1;
};

}
