/**
 * @file
 */

#pragma once

#include "Table.h"

namespace core {
class Tokenizer;
}

namespace databasetool {

extern bool parseConstraints(core::Tokenizer& tok, Table& table);
extern bool parseField(core::Tokenizer& tok, Table& table);
extern bool parseTable(core::Tokenizer& tok, Table& table);

}
