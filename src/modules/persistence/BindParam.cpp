/**
 * @file
 */

#include "BindParam.h"
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
		lengths.resize(position);
		formats.resize(position);
		fieldTypes.resize(position);
	}
	return index;
}

void BindParam::push(const Model& model, const Field& field) {
	const int index = add();
	fieldTypes[index] = field.type;
	if (model.isNull(field)) {
		values[index] = nullptr;
		Log::debug("Parameter %i: NULL", index + 1);
		return;
	}
	const bool notNull = field.nulloffset == -1;
	switch (field.type) {
	case FieldType::SHORT: {
		const int16_t value = notNull ? model.getValue<int16_t>(field) : *model.getValuePointer<int16_t>(field);
		valueBuffers.emplace_back(std::to_string(value));
		values[index] = valueBuffers.back().c_str();
		Log::debug("Parameter %i: '%s'", index + 1, values[index]);
		break;
	}
	case FieldType::BYTE: {
		const int8_t value = notNull ? model.getValue<uint8_t>(field) : *model.getValuePointer<uint8_t>(field);
		valueBuffers.emplace_back(std::to_string(value));
		values[index] = valueBuffers.back().c_str();
		Log::debug("Parameter %i: '%s'", index + 1, values[index]);
		break;
	}
	case FieldType::BLOB: {
		const Blob& value = notNull ? model.getValue<Blob>(field) : *model.getValuePointer<Blob>(field);
		values[index] = (const char*)value.data;
		lengths[index] = value.length;
		formats[index] = 1; // binary format
		Log::debug("Parameter %i: length: %i", index + 1, (int)value.length);
		break;
	}
	case FieldType::INT: {
		const int32_t value = notNull ? model.getValue<int32_t>(field) : *model.getValuePointer<int32_t>(field);
		valueBuffers.emplace_back(std::to_string(value));
		values[index] = valueBuffers.back().c_str();
		Log::debug("Parameter %i: '%s'", index + 1, values[index]);
		break;
	}
	case FieldType::DOUBLE: {
		const double value = notNull ? model.getValue<double>(field) : *model.getValuePointer<double>(field);
		valueBuffers.emplace_back(std::to_string(value));
		values[index] = valueBuffers.back().c_str();
		Log::debug("Parameter %i: '%s'", index + 1, values[index]);
		break;
	}
	case FieldType::LONG: {
		const int64_t value = notNull ? model.getValue<int64_t>(field) : *model.getValuePointer<int64_t>(field);
		valueBuffers.emplace_back(std::to_string(value));
		values[index] = valueBuffers.back().c_str();
		Log::debug("Parameter %i: '%s'", index + 1, values[index]);
		break;
	}
	case FieldType::BOOLEAN: {
		const bool value = notNull ? model.getValue<bool>(field) : *model.getValuePointer<bool>(field);
		if (value) {
			values[index] = "TRUE";
		} else {
			values[index] = "FALSE";
		}
		Log::debug("Parameter %i: '%s'", index + 1, values[index]);
		break;
	}
	case FieldType::TIMESTAMP: {
		const Timestamp& value = notNull ? model.getValue<Timestamp>(field) : *model.getValuePointer<Timestamp>(field);
		core_assert_msg(!value.isNow(), "'NOW()' timestamps are not pushed as parameters - but as NOW()");
		valueBuffers.emplace_back(std::to_string(value.seconds()));
		values[index] = valueBuffers.back().c_str();
		Log::debug("Parameter %i: '%s'", index + 1, values[index]);
		break;
	}
	case FieldType::PASSWORD:
	case FieldType::STRING:
	case FieldType::TEXT: {
		valueBuffers.emplace_back(notNull ? model.getValue<core::String>(field) : *model.getValuePointer<core::String>(field));
		values[index] = valueBuffers.back().c_str();
		Log::debug("Parameter %i: '%s'", index + 1, values[index]);
		break;
	}
	case FieldType::MAX:
		break;
	}
}

}
