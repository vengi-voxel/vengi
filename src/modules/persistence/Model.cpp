/**
 * @file
 */

#include "Model.h"
#include "ConnectionPool.h"
#include "ScopedConnection.h"
#include "core/Log.h"
#include "core/String.h"
#include <algorithm>

namespace persistence {

Model::Model(const std::string& tableName) :
		_tableName(tableName) {
	_membersPointer = (uint8_t*)this;
}

Model::~Model() {
	_fields.clear();
}

bool Model::isPrimaryKey(const std::string& fieldname) const {
	auto i = std::find_if(_fields.begin(), _fields.end(),
			[&fieldname](const Field& f) {return f.name == fieldname;}
	);
	if (i == _fields.end()) {
		return false;
	}

	return (i->contraintMask & std::enum_value(ConstraintType::PRIMARYKEY)) != 0;
}

Model::PreparedStatement Model::prepare(const std::string& name, const std::string& statement) {
	return PreparedStatement(this, name, statement);
}

bool Model::checkLastResult(State& state, Connection* connection) const {
	state.affectedRows = 0;
	if (state.res == nullptr) {
		Log::debug("Empty result");
		return false;
	}

	ExecStatusType lastState = PQresultStatus(state.res);

	switch (lastState) {
	case PGRES_NONFATAL_ERROR:
		state.lastErrorMsg = PQerrorMessage(connection->connection());
		Log::warn("non fatal error: %s", state.lastErrorMsg.c_str());
		break;
	case PGRES_BAD_RESPONSE:
	case PGRES_FATAL_ERROR:
		state.lastErrorMsg = PQerrorMessage(connection->connection());
		Log::error("fatal error: %s", state.lastErrorMsg.c_str());
		PQclear(state.res);
		state.res = nullptr;
		return false;
	case PGRES_EMPTY_QUERY:
	case PGRES_COMMAND_OK:
	case PGRES_TUPLES_OK:
		state.affectedRows = PQntuples(state.res);
		Log::debug("Affected rows %i", state.affectedRows);
		break;
	default:
		Log::error("not catched state: %s", PQresStatus(lastState));
		return false;
	}

	state.result = true;
	return true;
}

bool Model::exec(const char* query) {
	Log::debug("%s", query);
	ScopedConnection scoped(core::Singleton<ConnectionPool>::getInstance().connection());
	if (!scoped) {
		Log::error("Could not execute query '%s' - could not acquire connection", query);
		return false;
	}
	ConnectionType* conn = scoped.connection()->connection();
	State s(PQexec(conn, query));
	checkLastResult(s, scoped);
	return fillModelValues(s);
}

Model::Field Model::getField(const std::string& name) const {
	for (const Field& field : _fields) {
		if (field.name == name) {
			return field;
		}
	}
	return Field();
}

bool Model::begin() {
	return exec("START TRANSACTION;");
}

bool Model::commit() {
	return exec("COMMIT;");
}

bool Model::rollback() {
	return exec("ROLLBACK;");
}

bool Model::fillModelValues(Model::State& state) {
	if (state.affectedRows > 1) {
		Log::debug("More than one row affected, can't fill model values");
		return state.result;
	} else if (state.affectedRows <= 0) {
		Log::trace("No rows affected, can't fill model values");
		return state.result;
	}

	const int nFields = PQnfields(state.res);
	Log::trace("Query has values for %i fields", nFields);
	for (int i = 0; i < nFields; ++i) {
		const char* name = PQfname(state.res, i);
		const char* value = PQgetvalue(state.res, 0, i);
		if (value == nullptr) {
			value = "";
		}
		const Field& f = getField(name);
		if (f.name != name) {
			Log::error("Unknown field name for '%s'", name);
			state.result = false;
			return false;
		}
		Log::debug("Try to set '%s' to '%s'", name, value);
		switch (f.type) {
		case FieldType::STRING:
		case FieldType::PASSWORD:
			setValue(f, std::string(value));
			break;
		case FieldType::INT:
			setValue(f, core::string::toInt(value));
			break;
		case FieldType::LONG:
			setValue(f, core::string::toLong(value));
			break;
		case FieldType::TIMESTAMP: {
			setValue(f, Timestamp(core::string::toLong(value)));
			break;
		}
		}
	}
	return true;
}

Model::PreparedStatement::PreparedStatement(Model* model, const std::string& name, const std::string& statement) :
		_model(model), _name(name), _statement(statement) {
}

Model::State Model::PreparedStatement::exec() {
	Log::debug("prepared statement: '%s'", _statement.c_str());
	ScopedConnection scoped(core::Singleton<ConnectionPool>::getInstance().connection());
	if (!scoped) {
		Log::error("Could not prepare query '%s' - could not acquire connection", _statement.c_str());
		return State(nullptr);
	}

	ConnectionType* conn = scoped.connection()->connection();

	if (!scoped.connection()->hasPreparedStatement(_name)) {
		State state(PQprepare(conn, _name.c_str(), _statement.c_str(), (int)_params.size(), nullptr));
		if (!_model->checkLastResult(state, scoped)) {
			return state;
		}
		scoped.connection()->registerPreparedStatement(_name);
	}
	const int size = _params.size();
	const char* paramValues[size];
	for (int i = 0; i < size; ++i) {
		paramValues[i] = _params[i].first.c_str();
	}
	State prepState(PQexecPrepared(conn, _name.c_str(), size, paramValues, nullptr, nullptr, 0));
	if (!_model->checkLastResult(prepState, scoped)) {
		return prepState;
	}
	_model->fillModelValues(prepState);
	return prepState;
}

Model::State::State(ResultType* _res) :
		res(_res) {
}

Model::State::State(State&& other) :
		res(other.res), lastErrorMsg(other.lastErrorMsg), affectedRows(
				other.affectedRows), result(other.result) {
	other.res = nullptr;
}

Model::State::~State() {
	if (res != nullptr) {
		PQclear(res);
		res = nullptr;
	}
}

Model::ScopedTransaction::ScopedTransaction(Model* model, bool autocommit) :
		_autocommit(autocommit), _model(model) {
}

Model::ScopedTransaction::~ScopedTransaction() {
	if (_autocommit) {
		commit();
	} else {
		rollback();
	}
}

void Model::ScopedTransaction::commit() {
	if (_commited) {
		return;
	}
	_commited = true;
	_model->commit();
}

void Model::ScopedTransaction::rollback() {
	if (_commited) {
		return;
	}
	_commited = true;
	_model->rollback();
}

}
