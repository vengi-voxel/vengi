/**
 * @file
 */

#include "PreparedStatement.h"
#include "ScopedConnection.h"
#include "ConnectionPool.h"
#include "Connection.h"
#include "Model.h"
#include "FieldType.h"

#include "core/Singleton.h"
#include "core/Log.h"

namespace persistence {

PreparedStatement::BindParam::BindParam(int num) :
		values(num, nullptr), lengths(num, 0), formats(num, 0), fieldTypes(num, FieldType::INT) {
	valueBuffers.reserve(num);
}

int PreparedStatement::BindParam::add() {
	const int index = position;
	++position;
	if (values.capacity() < (size_t)position) {
		values.resize(position);
		valueBuffers.resize(position);
		lengths.resize(position);
		formats.resize(position);
		fieldTypes.resize(position);
	}
	return index;
}

PreparedStatement::PreparedStatement(Model* model, const std::string& name, const std::string& statement) :
		_model(model), _name(name), _statement(statement), _params(std::count(statement.begin(), statement.end(), '$')) {
}

State PreparedStatement::exec() {
	Log::debug("prepared statement: '%s'", _statement.c_str());
	ScopedConnection scoped(core::Singleton<ConnectionPool>::getInstance().connection());
	if (!scoped) {
		Log::error("Could not prepare query '%s' - could not acquire connection", _statement.c_str());
		return State();
	}

	if (_name.empty() || !scoped.connection()->hasPreparedStatement(_name)) {
		State state(scoped.connection());
		if (!state.prepare(_name.c_str(), _statement.c_str(), (int)_params.values.size())) {
			return state;
		}
	}

	const int size = _params.position;
	State prepState(scoped.connection());
	prepState.execPrepared(_name.c_str(), size, &_params.values[0]);
	if (!prepState.result) {
		return prepState;
	}
	if (prepState.affectedRows > 1) {
		Log::debug("More than one row affected, can't fill model values");
		return prepState;
	} else if (prepState.affectedRows <= 0) {
		Log::trace("No rows affected, can't fill model values");
		return prepState;
	}
	_model->fillModelValues(prepState);
	return prepState;
}

PreparedStatement& PreparedStatement::add(const std::string& value, FieldType fieldType) {
	const int index = _params.add();
	_params.valueBuffers.emplace_back(value);
	_params.fieldTypes[index] = fieldType;
	_params.values[index] = _params.valueBuffers.back().data();
	return *this;
}

PreparedStatement& PreparedStatement::add(const std::string& value) {
	return add(value, FieldType::STRING);
}

PreparedStatement& PreparedStatement::add(int value) {
	return add(std::to_string(value), FieldType::INT);
}

PreparedStatement& PreparedStatement::add(long value) {
	return add(std::to_string(value), FieldType::LONG);
}

PreparedStatement& PreparedStatement::addPassword(const std::string& password) {
	return add(password, FieldType::PASSWORD);
}

PreparedStatement& PreparedStatement::addPassword(const char* password) {
	const int index = _params.add();
	_params.fieldTypes[index] = FieldType::PASSWORD;
	_params.values[index] = password;
	_params.lengths[index] = strlen(password);
	return *this;
}

PreparedStatement& PreparedStatement::add(const char* value, FieldType fieldType) {
	const int index = _params.add();
	_params.fieldTypes[index] = fieldType;
	_params.values[index] = value;
	_params.lengths[index] = strlen(value);
	return *this;
}

PreparedStatement& PreparedStatement::add(nullptr_t value, FieldType fieldType) {
	const int index = _params.add();
	_params.fieldTypes[index] = fieldType;
	_params.values[index] = value;
	return *this;
}

PreparedStatement& PreparedStatement::add(const Timestamp& type) {
	if (type.isNow()) {
		return add("NOW()", FieldType::TIMESTAMP);
	}
	return add(std::to_string(type.time()), FieldType::TIMESTAMP);
}

}
