/**
 * @file
 */

#pragma once

#include <string>
#include <vector>
#include "Order.h"

namespace persistence {

class BindParam;
class Model;
class DBCondition;
class OrderBy;
struct Range;

extern std::string createCreateTableStatement(const Model& model);
extern std::string createTruncateTableStatement(const Model& model);
extern std::string createDropTableStatement(const Model& model);
extern std::string createUpdateStatement(const Model& model, BindParam* params = nullptr);
extern std::string createDeleteStatement(const Model& model);
extern std::string createInsertStatement(const Model& model, BindParam* params = nullptr);

extern std::string createSelect(const Model& model);
extern const char* createTransactionBegin();
extern const char* createTransactionCommit();
extern const char* createTransactionRollback();

extern std::string createWhere(const DBCondition& condition, int &parameterCount);
extern std::string createOrderBy(const OrderBy& orderBy);
extern std::string createLimitOffset(const Range& range);

}
