/**
 * @file
 */

#pragma once

#include <string>

namespace persistence {

class Model;

extern std::string createCreateTableStatement(const Model& model);
extern std::string createTruncateTableStatement(const Model& model);
extern std::string createInsertStatement(const Model& model);

extern std::string createSelect(const Model& model);

extern const char* createTransactionBegin();
extern const char* createTransactionCommit();
extern const char* createTransactionRollback();

}
