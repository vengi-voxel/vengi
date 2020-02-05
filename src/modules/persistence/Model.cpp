/**
 * @file
 */

#include "Model.h"
#include "ConstraintType.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/Singleton.h"
#include "core/Assert.h"
#include <algorithm>

namespace persistence {

core::String toLower(const char *in) {
	const core::String s(in);
	return s.toLower();
}

core::String toLower(const core::String& in) {
	return in.toLower();
}

Model::Model(const Meta* s) :
		_s(s) {
	_membersPointer = (uint8_t*)this;
}

Model::~Model() {
}

const Field& Model::getField(const char* name) const {
	if (name != nullptr && name[0] != '\0') {
		for (auto i = _s->_fields.begin(); i != _s->_fields.end(); ++i) {
			const Field& field = *i;
			if (field.name == name) {
				return field;
			}
		}
		// might e.g. happen in table update steps
		Log::debug("Failed to lookup field '%s' in table '%s'", name, _s->_tableName);
	}
	static const Field emptyField {};
	return emptyField;
}

bool Model::fillModelValues(State& state) {
	const int cols = state.cols;
	Log::debug("Query has values for %i cols", cols);
	for (int i = 0; i < cols; ++i) {
		const char* name = state.columnName(i);
		const Field& f = getField(name);
		if (f.name != name) {
			Log::error("Unknown field name for '%s'", name);
			state.result = false;
			return false;
		}
		const char *value;
		int length;
		bool isNull;
		state.getResult(i, f.type, &value, &length, &isNull);
		Log::debug("Try to set '%s' to '%s' (length: %i)", name, value, length);
		switch (f.type) {
		case FieldType::PASSWORD:
		case FieldType::TEXT:
			setValue(f, core::String(value, length));
			break;
		case FieldType::STRING: {
			const core::String s(value, length);
			if (f.isLower()) {
				setValue(f, s.toLower());
			} else {
				setValue(f, s);
			}
			break;
		}
		case FieldType::BOOLEAN:
			setValue(f, state.isBool(value));
			break;
		case FieldType::BLOB:
			setValue(f, Blob((uint8_t*)value, length));
			break;
		case FieldType::INT:
			setValue(f, (int32_t)core::string::toInt(value));
			break;
		case FieldType::SHORT:
			setValue(f, (int16_t)core::string::toInt(value));
			break;
		case FieldType::BYTE:
			setValue(f, (uint8_t)core::string::toInt(value));
			break;
		case FieldType::LONG:
			setValue(f, core::string::toLong(value));
			break;
		case FieldType::DOUBLE:
			setValue(f, core::string::toDouble(value));
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
	++state.currentRow;
	return true;
}

void Model::setValue(const Field& f, const core::String& value) {
	core_assert(f.offset >= 0);
	uint8_t* target = (uint8_t*)(_membersPointer + f.offset);
	core::String* targetValue = (core::String*)target;
	*targetValue = value;
	setValid(f, true);
}

void Model::setValue(const Field& f, const Timestamp& value) {
	core_assert(f.offset >= 0);
	uint8_t* target = (uint8_t*)(_membersPointer + f.offset);
	Timestamp* targetValue = (Timestamp*)target;
	*targetValue = value;
	setValid(f, true);
}

void Model::setValue(const Field& f, std::nullptr_t np) {
	setIsNull(f, true);
}

void Model::setIsNull(const Field& f, bool isNull) {
	if (f.nulloffset == -1) {
		return;
	}
	uint8_t* target = (uint8_t*)(_membersPointer + f.nulloffset);
	bool* targetValue = (bool*)target;
	*targetValue = isNull;
	setValid(f, true);
}

void Model::setValid(const Field& f, bool valid) {
	uint8_t* target = (uint8_t*)(_membersPointer + f.validoffset);
	bool* targetValue = (bool*)target;
	*targetValue = valid;
}

void Model::reset(const Field& f) {
	setValid(f, false);
}

bool Model::isValid(const Field& f) const {
	const uint8_t* target = (const uint8_t*)(_membersPointer + f.validoffset);
	const bool* targetValue = (const bool*)target;
	return *targetValue;
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
