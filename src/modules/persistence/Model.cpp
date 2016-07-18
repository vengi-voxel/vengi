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
}

bool Model::isPrimaryKey(const std::string& fieldname) const {
	auto i = std::find_if(_fields.begin(), _fields.end(),
			[&fieldname](const Field& f) {return f.name == fieldname;}
	);
	if (i == _fields.end()) {
		return false;
	}

	return (i->contraintMask & Model::PRIMARYKEY) != 0;
}

Model::PreparedStatement Model::prepare(const std::string& name, const std::string& statement) {
	return PreparedStatement(this, name, statement);
}

bool Model::checkLastResult(State& state, Connection* connection) const {
	state.affectedRows = 0;
	if (state.res == nullptr) {
		return false;
	}

	state.lastState = PQresultStatus(state.res);

	switch (state.lastState) {
	case PGRES_BAD_RESPONSE:
	case PGRES_NONFATAL_ERROR:
	case PGRES_FATAL_ERROR:
		state.lastErrorMsg = PQerrorMessage(*connection);
		Log::error("Failed to execute sql: %s ", state.lastErrorMsg.c_str());
		return false;
	case PGRES_EMPTY_QUERY:
	case PGRES_COMMAND_OK:
		state.affectedRows = 0;
		break;
	case PGRES_TUPLES_OK:
		state.affectedRows = PQntuples(state.res);
		Log::trace("Affected rows %i", state.affectedRows);
		break;
	default:
		Log::error("not catched state: %s", PQresStatus(state.lastState));
		return false;
	}

	state.result = true;
	return true;
}

bool Model::exec(const char* query) {
	Log::debug("%s", query);
	ScopedConnection scoped(ConnectionPool::get().connection());
	if (scoped == false) {
		Log::error("Could not execute query '%s' - could not acquire connection", query);
		return false;
	}
	State s(PQexec(scoped, query));
	checkLastResult(s, scoped);
	return s.result;
}

Model::Field Model::getField(const std::string& name) const {
	for (const Field& field : _fields) {
		if (field.name == name) {
			return field;
		}
	}
	return Field();
}

Model::PreparedStatement::PreparedStatement(Model* model, const std::string& name, const std::string& statement, const std::vector<std::string>& params) :
		_model(model), _name(name), _statement(statement), _params(params) {
}

Model::State Model::PreparedStatement::exec() {
	ScopedConnection scoped(ConnectionPool::get().connection());
	if (scoped == false) {
		Log::error("Could not prepare query '%s' - could not acquire connection", _statement.c_str());
		return State(nullptr);
	}
	State state = State(PQprepare(scoped, _name.c_str(), _statement.c_str(), (int)_params.size(), nullptr));
	if (!_model->checkLastResult(state, scoped)) {
		return state;
	}
	const int size = _params.size();
	const char* paramValues[size];
	for (int i = 0; i < size; ++i) {
		paramValues[i] = _params[i].c_str();
	}
	State prepState(PQexecPrepared(scoped, _name.c_str(), size, paramValues, nullptr, nullptr, 0));
	if (!_model->checkLastResult(prepState, scoped)) {
		return prepState;
	}
	if (state.affectedRows == 1) {
		const int nFields = PQnfields(state.res);
		for (int i = 0; i < nFields; ++i) {
			const char* name = PQfname(state.res, i);
			const char* value = PQgetvalue(state.res, 0, i);
			const Field& f = _model->getField(name);
			if (f.name != name) {
				Log::error("Unkown field name for '%s'", name);
				prepState.result = false;
				return prepState;
			}
			Log::debug("Try to set '%s' to '%s'", name, value);
			switch (f.type) {
			case Model::STRING:
			case Model::PASSWORD:
				_model->setValue(f, std::string(value));
				break;
			case Model::INT:
				_model->setValue(f, core::string::toInt(value));
				break;
			case Model::LONG:
				_model->setValue(f, core::string::toLong(value));
				break;
			case Model::TIMESTAMP:
				// TODO: implement me
				break;
			}
		}
	}
	return prepState;
}

Model::State::State(PGresult* _res) :
		res(_res) {
}

Model::State::~State() {
	if (res != nullptr) {
		PQclear(res);
	}
}

}
