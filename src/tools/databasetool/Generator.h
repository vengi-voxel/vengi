/**
 * @file
 */

#pragma once

#include "Table.h"
#include "core/String.h"

namespace databasetool {

extern bool generateClassForTable(const databasetool::Table& table, core::String& src);

}
