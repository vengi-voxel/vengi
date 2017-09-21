#include "Util.h"
#include "Table.h"

namespace databasetool {

bool needsInitCPP(persistence::Model::FieldType type) {
	switch (type) {
	case persistence::Model::FieldType::PASSWORD:
	case persistence::Model::FieldType::STRING:
	case persistence::Model::FieldType::TIMESTAMP:
		return false;
	default:
		return true;
	}
}

std::string getCPPInit(persistence::Model::FieldType type, bool pointer) {
	if (pointer) {
		return "nullptr";
	}
	switch (type) {
	case persistence::Model::FieldType::PASSWORD:
	case persistence::Model::FieldType::STRING:
		return "\"\"";
	case persistence::Model::FieldType::TIMESTAMP:
	case persistence::Model::FieldType::LONG:
		return "0l";
	case persistence::Model::FieldType::INT:
		return "0";
	case persistence::Model::FieldType::MAX:
		break;
	}
	return "";
}

std::string getCPPType(persistence::Model::FieldType type, bool function, bool pointer) {
	switch (type) {
	case persistence::Model::FieldType::PASSWORD:
	case persistence::Model::FieldType::STRING:
		if (pointer) {
			return "const char*";
		}
		if (function) {
			return "const std::string&";
		}
		return "std::string";
	case persistence::Model::FieldType::TIMESTAMP:
		if (function) {
			if (pointer) {
				return "const ::persistence::Timestamp*";
			}
			return "const ::persistence::Timestamp&";
		}
		return "::persistence::Timestamp";
	case persistence::Model::FieldType::LONG:
		if (pointer) {
			return "const int64_t*";
		}
		return "int64_t";
	case persistence::Model::FieldType::INT:
		if (pointer) {
			return "const int32_t*";
		}
		return "int32_t";
	case persistence::Model::FieldType::MAX:
		break;
	}
	return "";
}

std::string getDbType(const persistence::Model::Field& field) {
	switch (field.type) {
	case persistence::Model::FieldType::PASSWORD:
	case persistence::Model::FieldType::STRING: {
		std::string type = "CHAR(";
		if (field.length > 0) {
			type += std::to_string(field.length);
		} else {
			type += "256";
		}
		type += ')';
		return type;
	}
	case persistence::Model::FieldType::TIMESTAMP:
		return "TIMESTAMP";
	case persistence::Model::FieldType::LONG:
		if (field.isAutoincrement()) {
			return "";
		}
		return "BIGINT";
	case persistence::Model::FieldType::INT:
		if (field.isAutoincrement()) {
			return "";
		}
		return "INT";
	case persistence::Model::FieldType::MAX:
		break;
	}
	return "";
}

std::string getDbFlags(const Table& table, const persistence::Model::Field& field) {
	std::stringstream ss;
	if (field.isAutoincrement()) {
		if (field.type == persistence::Model::FieldType::LONG) {
			ss << " BIGSERIAL";
		} else {
			ss << " SERIAL";
		}
	}
	if (field.isNotNull()) {
		ss << " NOT NULL";
	}
	if (field.isPrimaryKey() && table.primaryKeys == 1) {
		ss << " PRIMARY KEY";
	}
	if (field.isUnique()) {
		auto i = table.contraints.find(field.name);
		// only if there is one field in the unique list - otherwise we have to construct
		// them differently like the primary key for multiple fields
		if (i == table.contraints.end() || i->second.fields.size() == 1) {
			ss << " UNIQUE";
		}
	}
	if (!field.defaultVal.empty()) {
		ss << " DEFAULT " << field.defaultVal;
	}
	return ss.str();
}

void sep(std::stringstream& ss, int count) {
	ss << "$" << count;
}

void sort(databasetool::Fields& fields) {
	// TODO: implement me
}

bool isPointer(const persistence::Model::Field& field) {
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
