/**
 * @file
 */

#pragma once

#include "Model.h"
#include <string>

namespace persistence {

extern std::string createCreateTableStatement(const Model& model);
extern std::string createTruncateTableStatement(const Model& model);
extern std::string createInsertStatement(const Model& model);

extern std::string createSelect(const Model& model);
extern std::string createSelect(const Fields& fields, const std::string& tableName);

extern const char* createTransactionBegin();
extern const char* createTransactionCommit();
extern const char* createTransactionRollback();

}
