/**
 * @file
 */

#include "BindParam.h"
#include "ScopedConnection.h"
#include "ConnectionPool.h"
#include "Connection.h"
#include "Model.h"
#include "FieldType.h"

#include "core/Singleton.h"
#include "core/Log.h"

namespace persistence {

BindParam::BindParam(int num) :
		values(num, nullptr), lengths(num, 0), formats(num, 0), fieldTypes(num, FieldType::INT) {
	valueBuffers.reserve(num);
}

int BindParam::add() {
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

void BindParam::push(const Model& model, const Field& field) {
	const int index = add();
	switch (field.type) {
	case FieldType::SHORT: {
		const int16_t value = model.getValue<int16_t>(field);
		valueBuffers.emplace_back(std::to_string(value));
		values[index] = valueBuffers.back().c_str();
		Log::debug("Parameter %i: '%s'", index + 1, values[index]);
		break;
	}
	case FieldType::BYTE: {
		const int8_t value = model.getValue<uint8_t>(field);
		valueBuffers.emplace_back(std::to_string(value));
		values[index] = valueBuffers.back().c_str();
		Log::debug("Parameter %i: '%s'", index + 1, values[index]);
		break;
	}
	case FieldType::INT: {
		const int32_t value = model.getValue<int32_t>(field);
		valueBuffers.emplace_back(std::to_string(value));
		values[index] = valueBuffers.back().c_str();
		Log::debug("Parameter %i: '%s'", index + 1, values[index]);
		break;
	}
	case FieldType::LONG: {
		const int64_t value = model.getValue<int64_t>(field);
		valueBuffers.emplace_back(std::to_string(value));
		values[index] = valueBuffers.back().c_str();
		Log::debug("Parameter %i: '%s'", index + 1, values[index]);
		break;
	}
	case FieldType::BOOLEAN: {
		const bool value = model.getValue<bool>(field);
		if (value) {
			values[index] = "TRUE";
		} else {
			values[index] = "FALSE";
		}
		Log::debug("Parameter %i: '%s'", index + 1, values[index]);
		break;
	}
	case FieldType::TIMESTAMP: {
		const Timestamp& value = model.getValue<Timestamp>(field);
		core_assert_msg(!value.isNow(), "'NOW()' timestamps are not pushed as parameters - but as NOW()");
		valueBuffers.emplace_back(std::to_string(value.seconds()));
		values[index] = valueBuffers.back().c_str();
		Log::debug("Parameter %i: '%s'", index + 1, values[index]);
		break;
	}
	case FieldType::PASSWORD:
	case FieldType::STRING:
	case FieldType::TEXT: {
		if (field.isNotNull()) {
			valueBuffers.emplace_back(model.getValue<std::string>(field));
			values[index] = valueBuffers.back().c_str();
		} else {
			values[index] = model.getValue<const char*>(field);
		}
		Log::debug("Parameter %i: '%s'", index + 1, values[index]);
		break;
	}
	case FieldType::MAX:
		break;
	}
	fieldTypes[index] = field.type;
}

}
