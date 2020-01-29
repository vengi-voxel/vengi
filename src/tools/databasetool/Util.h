/**
 * @file
 */

#pragma once

#include "Table.h"
#include "persistence/FieldType.h"
#include "core/String.h"
#include <sstream>

namespace databasetool {

extern bool needsInitCPP(persistence::FieldType type);
extern std::string getCPPType(persistence::FieldType type, bool function = false, bool pointer = false);
extern std::string getCPPInit(persistence::FieldType type, bool pointer);
extern void sep(std::stringstream& ss, int count);
extern void sort(databasetool::Fields& fields);
extern bool isPointer(const persistence::Field& field);
extern bool isString(const persistence::Field& field);

}
