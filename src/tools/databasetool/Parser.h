/**
 * @file
 */

#pragma once

#include "core/Tokenizer.h"
#include "Table.h"

namespace databasetool {

extern bool parseConstraints(core::Tokenizer& tok, Table& table);
extern bool parseField(core::Tokenizer& tok, Table& table);
extern bool parseTable(core::Tokenizer& tok, Table& table);

}
