#include "Util.h"
#include "Table.h"
#include "core/String.h"

namespace databasetool {

bool needsInitCPP(persistence::FieldType type) {
	switch (type) {
	case persistence::FieldType::PASSWORD:
	case persistence::FieldType::STRING:
	case persistence::FieldType::TEXT:
	case persistence::FieldType::TIMESTAMP:
		return false;
	default:
		return true;
	}
}

std::string getCPPInit(persistence::FieldType type, bool pointer) {
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
	case persistence::FieldType::INT:
	case persistence::FieldType::SHORT:
		return "0";
	case persistence::FieldType::MAX:
		break;
	}
	return "";
}

std::string getCPPType(persistence::FieldType type, bool function, bool pointer) {
	switch (type) {
	case persistence::FieldType::BOOLEAN:
		return "bool";
	case persistence::FieldType::PASSWORD:
	case persistence::FieldType::STRING:
	case persistence::FieldType::TEXT:
		if (pointer) {
			return "const char*";
		}
		if (function) {
			return "const std::string&";
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
