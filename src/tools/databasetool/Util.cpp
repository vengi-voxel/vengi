#include "Util.h"
#include "core/StringUtil.h"

namespace databasetool {

bool needsInitCPP(persistence::FieldType type) {
	switch (type) {
	case persistence::FieldType::PASSWORD:
	case persistence::FieldType::STRING:
	case persistence::FieldType::TEXT:
	case persistence::FieldType::TIMESTAMP:
	case persistence::FieldType::BLOB:
		return false;
	default:
		return true;
	}
}

core::String getCPPInit(persistence::FieldType type, bool pointer) {
	if (pointer) {
		return "nullptr";
	}
	switch (type) {
	case persistence::FieldType::BOOLEAN:
		return "false";
	case persistence::FieldType::TEXT:
	case persistence::FieldType::PASSWORD:
	case persistence::FieldType::STRING:
		return "\"\"";
	case persistence::FieldType::TIMESTAMP:
	case persistence::FieldType::LONG:
		return "0l";
	case persistence::FieldType::DOUBLE:
		return "0.0";
	case persistence::FieldType::INT:
	case persistence::FieldType::SHORT:
		return "0";
	case persistence::FieldType::BYTE:
		return "0u";
	case persistence::FieldType::BLOB:
	case persistence::FieldType::MAX:
		break;
	}
	return "";
}

core::String getCPPType(persistence::FieldType type, bool function, bool pointer) {
	switch (type) {
	case persistence::FieldType::BOOLEAN:
		if (pointer) {
			return "const bool*";
		}
		return "bool";
	case persistence::FieldType::PASSWORD:
	case persistence::FieldType::STRING:
	case persistence::FieldType::TEXT:
		if (pointer) {
			return "const char*";
		}
		if (function) {
			return "const core::String&";
		}
		return "std::string";
	case persistence::FieldType::TIMESTAMP:
		if (function) {
			if (pointer) {
				return "const persistence::Timestamp*";
			}
			return "const persistence::Timestamp&";
		}
		return "persistence::Timestamp";
	case persistence::FieldType::LONG:
		if (pointer) {
			return "const int64_t*";
		}
		return "int64_t";
	case persistence::FieldType::DOUBLE:
		if (pointer) {
			return "const double*";
		}
		return "double";
	case persistence::FieldType::INT:
		if (pointer) {
			return "const int32_t*";
		}
		return "int32_t";
	case persistence::FieldType::SHORT:
		if (pointer) {
			return "const int16_t*";
		}
		return "int16_t";
	case persistence::FieldType::BYTE:
		if (pointer) {
			return "const int8_t*";
		}
		return "int8_t";
	case persistence::FieldType::BLOB:
		if (pointer) {
			return "const persistence::Blob*";
		}
		return "persistence::Blob";
	case persistence::FieldType::MAX:
		break;
	}
	return "";
}

void sep(std::stringstream& ss, int count) {
	ss << "$" << count;
}

void sort(databasetool::Fields& fields) {
	// TODO: implement me
}

bool isString(const persistence::Field& field) {
	return field.type == persistence::FieldType::TEXT || field.type == persistence::FieldType::STRING || field.type == persistence::FieldType::PASSWORD;
}

bool isPointer(const persistence::Field& field) {
	if (field.isNotNull()) {
		return false;
	}
	if (field.isPrimaryKey()) {
		return false;
	}
	if (field.isAutoincrement()) {
		return false;
	}
	if (field.isUnique()) {
		return false;
	}
	return true;
}

}
