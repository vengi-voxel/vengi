/**
 * @file
 */

#pragma once

#include <string>
#include <vector>

namespace persistence {

class BindParam;
class Model;
class DBCondition;

extern std::string createCreateTableStatement(const Model& model);
extern std::string createTruncateTableStatement(const Model& model);
extern std::string createUpdateStatement(const Model& model, BindParam* params = nullptr);
extern std::string createDeleteStatement(const Model& model);
extern std::string createInsertStatement(const Model& model, BindParam* params = nullptr);

extern std::string createSelect(const Model& model);
extern const char* createTransactionBegin();
extern const char* createTransactionCommit();
extern const char* createTransactionRollback();

extern std::string createWhere(const DBCondition& condition, int &parameterCount);

}
