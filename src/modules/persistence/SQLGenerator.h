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

extern core::String createTableExistsStatement(const Model& model, BindParam* params);
extern core::String createAlterTableStatement(const std::vector<db::MetainfoModel>& columns, const Model& table, bool useForeignKeys);
extern core::String createCreateTableStatement(const Model& model, bool useForeignKeys);
extern core::String createTruncateTableStatement(const Model& model);
extern core::String createDropTableStatement(const Model& model);
extern core::String createUpdateStatement(const Model& model, BindParam* params = nullptr, int* parameterCount = nullptr);
extern core::String createDeleteStatement(const Model& model, BindParam* params = nullptr);
extern core::String createInsertBaseStatement(const Model& table, bool& primaryKeyIncluded);
extern core::String createInsertValuesStatement(const Model& table, BindParam* params, int& insertValueIndex);
extern core::String createInsertStatement(const Model& model, BindParam* params = nullptr, int* parameterCount = nullptr);
extern core::String createInsertStatement(const std::vector<const Model*>& tables, BindParam* params = nullptr, int* parameterCount = nullptr);

extern core::String createSelect(const Model& model, BindParam* params = nullptr);
extern const char* createTransactionBegin();
extern const char* createTransactionCommit();
extern const char* createTransactionRollback();

extern core::String createWhere(const DBCondition& condition, int &parameterCount);
extern core::String createOrderBy(const OrderBy& orderBy);
extern core::String createLimitOffset(const Range& range);

}
