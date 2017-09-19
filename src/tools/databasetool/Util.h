/**
 * @file
 */

#pragma once

#include "Table.h"
#include "persistence/Model.h"
#include <sstream>

namespace databasetool {

extern bool needsInitCPP(persistence::Model::FieldType type);
extern std::string getDbType(const persistence::Model::Field& field);
extern std::string getDbFlags(const Table& table, const persistence::Model::Field& field);
extern std::string getCPPType(persistence::Model::FieldType type, bool function = false, bool pointer = false);
extern std::string getCPPInit(persistence::Model::FieldType type, bool pointer);
extern void sep(std::stringstream& ss, int count);
extern void sort(databasetool::Fields& fields);
extern bool isPointer(const persistence::Model::Field& field);

}
