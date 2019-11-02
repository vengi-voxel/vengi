/**
 * @file
 */

#include "DBHandler.h"
#include "core/Singleton.h"
#include "core/Assert.h"
#include "ConnectionPool.h"
#include "core/Log.h"
#include "postgres/PQSymbol.h"

namespace persistence {

DBHandler::DBHandler(bool useForeignKeys) :
		_useForeignKeys(useForeignKeys) {
}

DBHandler::~DBHandler() {
	core_assert_always(!_initialized);
}

bool DBHandler::init() {
	if (_initialized) {
		return true;
	}
	if (!postgresInit()) {
		Log::error("Database driver initialization failed.");
		return false;
	}
	if (!core::Singleton<ConnectionPool>::getInstance().init()) {
		Log::error(logid, "Failed to init the connection pool");
		return false;
	}
	_initialized = createOrUpdateTable(db::MetainfoModel());
	return _initialized;
}

void DBHandler::shutdown() {
	_initialized = false;
	core::Singleton<ConnectionPool>::getInstance().shutdown();
	postgresShutdown();
}

Connection* DBHandler::connection() const {
	return core::Singleton<ConnectionPool>::getInstance().connection();
}

bool DBHandler::update(Model& model, const DBCondition& condition) const {
	BindParam params(10);
	const std::string& query = createUpdateStatement(model, &params);
	int conditionAmount = params.position;
	const std::string& where = createWhere(condition, conditionAmount);
	return execInternalWithParameters(query + where, model, params).result;
}

bool DBHandler::insert(Model& model) const {
	BindParam param(10);
	const std::string& query = createInsertStatement(model, &param);
	return execInternalWithParameters(query, model, param).result;
}

bool DBHandler::insert(Model&& model) const {
	BindParam param(10);
	const std::string& query = createInsertStatement(model, &param);
	return execInternalWithParameters(query, model, param).result;
}

bool DBHandler::insert(std::vector<const Model*>& models) const {
	BindParam param(10 * models.size());
	const std::string& query = createInsertStatement(models, &param);
	return execInternalWithParameters(query, param).result;
}

bool DBHandler::deleteModels(std::vector<const Model*>& models) const {
	bool state = true;
	// TODO: prepared statement
	for (const Model* m : models) {
		state &= deleteModel(*m);
	}
	return state;
}

bool DBHandler::truncate(const Model& model) const {
	return exec(createTruncateTableStatement(model));
}

bool DBHandler::truncate(Model&& model) const {
	return exec(createTruncateTableStatement(model));
}

MassQuery DBHandler::massQuery() const {
	return MassQuery(this);
}

bool DBHandler::dropTable(const Model& model) const {
	const State& s = execInternal(createDropTableStatement(model));
	if (!s.result) {
		return false;
	}
	const db::DBConditionMetainfoModelSchemaname c1(model.schema());
	const db::DBConditionMetainfoModelTablename c2(model.tableName());
	const DBConditionMultiple condition(true, { &c1, &c2 });
	deleteModel(db::MetainfoModel(), condition);
	return true;
}

bool DBHandler::dropTable(Model&& model) const {
	return dropTable(model);
}

bool DBHandler::tableExists(const Model& model) const {
	BindParam param(2);
	const std::string& stmt = createTableExistsStatement(model, &param);
	const State& s = execInternalWithParameters(stmt, param);
	core_assert(s.result);
	core_assert_msg(s.affectedRows == 1, "There should exactly be 1 affected row for this statement, but we got %i", s.affectedRows);
	core_assert_msg(s.cols == 1, "There should exactly be 1 affected column for this statement, but we got %i", s.cols);
	core_assert_msg(s.currentRow == 0, "s.currentRow should have been 0 - but is %i", s.currentRow);
	const bool result = s.asBool(0);
	Log::debug("check whether table '%s' exists: '%s'", model.tableName(), result ? "true" : "false");
	return result;
}

bool DBHandler::createOrUpdateTable(Model&& model) const {
	std::vector<db::MetainfoModel> schemaModels;
	if (!tableExists(model) || !loadMetadata(model, schemaModels)) {
		// doesn't exist yet - just create it
		if (!exec(createCreateTableStatement(model, _useForeignKeys))) {
			return false;
		}
	} else if (!exec(createAlterTableStatement(schemaModels, model, _useForeignKeys))) {
		return false;
	}
	return insertMetadata(model);
}

bool DBHandler::loadMetadata(const Model& model, std::vector<db::MetainfoModel>& schemaModels) const {
	schemaModels.reserve(model.fields().size() * 2);
	const db::DBConditionMetainfoModelSchemaname c1(model.schema());
	const db::DBConditionMetainfoModelTablename c2(model.tableName());
	const DBConditionMultiple condition(true, { &c1, &c2 });
	select(db::MetainfoModel(), condition, [&] (db::MetainfoModel&& model) {
		schemaModels.emplace_back(std::move(model));
	});
	return !schemaModels.empty();
}

bool DBHandler::insertMetadata(const Model& model) const {
	const db::DBConditionMetainfoModelSchemaname c1(model.schema());
	const db::DBConditionMetainfoModelTablename c2(model.tableName());
	const DBConditionMultiple condition(true, { &c1, &c2 });
	deleteModel(db::MetainfoModel(), condition);

	const Fields& fields = model.fields();
	std::vector<db::MetainfoModel> models;
	models.reserve(fields.size());
	for (const auto& f : fields) {
		db::MetainfoModel metaInfo;
		metaInfo.setMaximumlength(f.length);
		metaInfo.setColumndefault(f.defaultVal);
		metaInfo.setColumnname(f.name);
		metaInfo.setTablename(model.tableName());
		metaInfo.setSchemaname(model.schema());
		metaInfo.setConstraintmask(f.contraintMask);
		metaInfo.setDatatype(toFieldType(f.type));
		models.push_back(metaInfo);
	}
	return insert(models);
}

bool DBHandler::createTable(Model&& model) const {
	if (!exec(createCreateTableStatement(model, _useForeignKeys))) {
		Log::error(logid, "Failed to create table");
		return false;
	}
	insertMetadata(model);
	return true;
}

bool DBHandler::exec(const std::string& query) const {
	return execInternal(query).result;
}

State DBHandler::execInternal(const std::string& query) const {
	ScopedConnection scoped(connection());
	if (!scoped) {
		Log::error(logid, "Could not execute query '%s' - could not acquire connection", query.c_str());
		return State();
	}
	State s(scoped.connection());
	if (!s.exec(query.c_str())) {
		Log::warn(logid, "Failed to execute query: '%s'", query.c_str());
	} else {
		Log::debug(logid, "Executed query: '%s'", query.c_str());
	}
	return s;
}

State DBHandler::execInternalWithCondition(const std::string& query, BindParam& params, int conditionOffset, const DBCondition& condition) const {
	Log::debug(logid, "Execute query '%s'", query.c_str());
	ScopedConnection scoped(connection());
	if (!scoped) {
		Log::error("Could not execute query '%s' - could not acquire connection", query.c_str());
		return State();
	}
	State s(scoped.connection());
	if (conditionOffset > 0) {
		for (int i = 0; i < conditionOffset; ++i) {
			const int index = params.add();
			const char* value = condition.value(i);
			Log::debug(logid, "Parameter %i: '%s'", index + 1, value);
			params.values[index] = value;
		}
		if (!s.exec(query.c_str(), params.position, &params.values[0])) {
			Log::error("Failed to execute query '%s' with %i parameters", query.c_str(), conditionOffset);
		}
	} else if (!s.exec(query.c_str())) {
		Log::error("Failed to execute query '%s'", query.c_str());
	}
	if (s.affectedRows <= 0) {
		Log::trace(logid, "No rows affected.");
		return s;
	}
	return s;
}

State DBHandler::execInternalWithParameters(const std::string& query, Model& model, const BindParam& param) const {
	ScopedConnection scoped(connection());
	if (!scoped) {
		Log::error(logid, "Could not execute query '%s' - could not acquire connection", query.c_str());
		return State();
	}
	State s(scoped.connection());
	Log::debug("Execute query '%s' with %i parameters", query.c_str(), param.position);
	if (!s.exec(query.c_str(), param.position, &param.values[0])) {
		Log::warn(logid, "Failed to execute query: '%s'", query.c_str());
	}
	if (s.affectedRows <= 0) {
		Log::trace(logid, "No rows affected, can't fill model values");
		return s;
	}
	model.fillModelValues(s);
	return s;
}

State DBHandler::execInternalWithParameters(const std::string& query, const BindParam& param) const {
	ScopedConnection scoped(connection());
	if (!scoped) {
		Log::error(logid, "Could not execute query '%s' - could not acquire connection", query.c_str());
		return State();
	}
	State s(scoped.connection());
	Log::debug("Execute query '%s' with %i parameters", query.c_str(), param.position);
	if (!s.exec(query.c_str(), param.position, &param.values[0])) {
		Log::warn(logid, "Failed to execute query: '%s'", query.c_str());
	}
	Log::debug("current row: %i", s.currentRow);
	return s;
}

bool DBHandler::begin() {
	return exec(createTransactionBegin());
}

bool DBHandler::commit() {
	return exec(createTransactionCommit());
}

bool DBHandler::rollback() {
	return exec(createTransactionRollback());
}

}
