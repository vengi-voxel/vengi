/**
 * @file
 */

#pragma once

#include "core/String.h"
#include <vector>
#include "Order.h"
#include "MetainfoModel.h"

namespace persistence {

struct BindParam;
class Model;
class DBCondition;
class OrderBy;
struct Range;

extern std::string createTableExistsStatement(const Model& model, BindParam* params);
extern std::string createAlterTableStatement(const std::vector<db::MetainfoModel>& columns, const Model& table, bool useForeignKeys);
extern std::string createCreateTableStatement(const Model& model, bool useForeignKeys);
extern std::string createTruncateTableStatement(const Model& model);
extern std::string createDropTableStatement(const Model& model);
extern std::string createUpdateStatement(const Model& model, BindParam* params = nullptr, int* parameterCount = nullptr);
extern std::string createDeleteStatement(const Model& model, BindParam* params = nullptr);
extern std::string createInsertBaseStatement(const Model& table, bool& primaryKeyIncluded);
extern std::string createInsertValuesStatement(const Model& table, BindParam* params, int& insertValueIndex);
extern std::string createInsertStatement(const Model& model, BindParam* params = nullptr, int* parameterCount = nullptr);
extern std::string createInsertStatement(const std::vector<const Model*>& tables, BindParam* params = nullptr, int* parameterCount = nullptr);

extern std::string createSelect(const Model& model, BindParam* params = nullptr);
extern const char* createTransactionBegin();
extern const char* createTransactionCommit();
extern const char* createTransactionRollback();

extern std::string createWhere(const DBCondition& condition, int &parameterCount);
extern std::string createOrderBy(const OrderBy& orderBy);
extern std::string createLimitOffset(const Range& range);

}
