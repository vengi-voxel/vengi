/**
 * @file
 */

#include "Model.h"
#include "ConnectionPool.h"
#include "ScopedConnection.h"
#include "ConstraintType.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/Singleton.h"
#include <algorithm>

#include <libpq-fe.h>

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

bool Model::exec(const char* query) {
	Log::debug("%s", query);
	ScopedConnection scoped(core::Singleton<ConnectionPool>::getInstance().connection());
	if (!scoped) {
		Log::error("Could not execute query '%s' - could not acquire connection", query);
		return false;
	}
	State s(scoped.connection());
	s.exec(query);
	if (s.affectedRows > 1) {
		Log::debug("More than one row affected, can't fill model values");
		return s.result;
	} else if (s.affectedRows <= 0) {
		Log::trace("No rows affected, can't fill model values");
		return s.result;
	}
	return fillModelValues(s);
}

bool Model::exec(const char* query) const {
	Log::debug("%s", query);
	ScopedConnection scoped(core::Singleton<ConnectionPool>::getInstance().connection());
	if (!scoped) {
		Log::error("Could not execute query '%s' - could not acquire connection", query);
		return false;
	}
	State s(scoped.connection());
	return s.exec(query);
}

const Field& Model::getField(const std::string& name) const {
	for (const Field& field : _fields) {
		if (field.name == name) {
			return field;
		}
	}
	static const Field emptyField;
	return emptyField;
}

bool Model::fillModelValues(State& state) {
	const int cols = PQnfields(state.res);
	Log::trace("Query has values for %i cols", cols);
	for (int i = 0; i < cols; ++i) {
		const char* name = PQfname(state.res, i);
		const Field& f = getField(name);
		if (f.name != name) {
			Log::error("Unknown field name for '%s'", name);
			state.result = false;
			return false;
		}
		const bool isNull = PQgetisnull(state.res, state.currentRow, i);
		const char* value = isNull ? nullptr : PQgetvalue(state.res, state.currentRow, i);
		int length = PQgetlength(state.res, state.currentRow, i);
		if (value == nullptr) {
			value = "";
			length = 0;
		}
		Log::debug("Try to set '%s' to '%s' (length: %i)", name, value, length);
		switch (f.type) {
		case FieldType::TEXT:
		case FieldType::STRING:
		case FieldType::PASSWORD:
			setValue(f, std::string(value, length));
			break;
		case FieldType::BOOLEAN: {
			setValue(f, *value == '1' || *value == 't' || *value == 'y' || *value == 'o' || *value == 'T');
			break;
		}
		case FieldType::INT:
		case FieldType::SHORT:
			setValue(f, core::string::toInt(value));
			break;
		case FieldType::LONG:
			setValue(f, core::string::toLong(value));
			break;
		case FieldType::TIMESTAMP: {
			setValue(f, Timestamp(core::string::toLong(value)));
			break;
		}
		case FieldType::MAX:
			break;
		}
		setIsNull(f, isNull);
	}
	return true;
}

void Model::setValue(const Field& f, const std::string& value) {
	core_assert(f.offset >= 0);
	uint8_t* target = (uint8_t*)(_membersPointer + f.offset);
	std::string* targetValue = (std::string*)target;
	*targetValue = value;
}

void Model::setValue(const Field& f, const Timestamp& value) {
	core_assert(f.offset >= 0);
	uint8_t* target = (uint8_t*)(_membersPointer + f.offset);
	Timestamp* targetValue = (Timestamp*)target;
	*targetValue = value;
}

void Model::setIsNull(const Field& f, bool isNull) {
	if (f.nulloffset == -1) {
		return;
	}
	uint8_t* target = (uint8_t*)(_membersPointer + f.nulloffset);
	bool* targetValue = (bool*)target;
	*targetValue = isNull;
}

bool Model::isNull(const Field& f) const {
	if (f.nulloffset < 0) {
		return false;
	}
	const uint8_t* target = (const uint8_t*)(_membersPointer + f.nulloffset);
	const bool* targetValue = (const bool*)target;
	return *targetValue;
}

}
