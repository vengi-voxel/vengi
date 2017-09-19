#pragma once

#include "Table.h"
#include <sstream>

namespace databasetool {

extern bool generateClassForTable(const databasetool::Table& table, std::stringstream& src);

}
