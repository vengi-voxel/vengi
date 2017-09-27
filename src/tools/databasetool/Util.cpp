#include "Util.h"
#include "Table.h"
#include "core/String.h"

namespace databasetool {

bool needsInitCPP(persistence::Model::FieldType type) {
	switch (type) {
	case persistence::Model::FieldType::PASSWORD:
	case persistence::Model::FieldType::STRING:
	case persistence::Model::FieldType::TEXT:
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
	case persistence::Model::FieldType::TEXT:
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
	case persistence::Model::FieldType::TEXT:
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
				return "const persistence::Timestamp*";
			}
			return "const persistence::Timestamp&";
		}
		return "persistence::Timestamp";
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

std::string getDbFlags(int numberPrimaryKeys, const persistence::Model::Constraints& constraints, const persistence::Model::Field& field) {
	std::stringstream ss;
	bool empty = true;
	if (field.isAutoincrement()) {
		empty = false;
		if (field.type == persistence::Model::FieldType::LONG) {
			ss << "BIGSERIAL";
		} else {
			ss << "SERIAL";
		}
	}
	if (field.isNotNull()) {
		if (!empty) {
			ss << " ";
		}
		ss << "NOT NULL";
		empty = false;
	}
	if (field.isPrimaryKey() && numberPrimaryKeys == 1) {
		if (!empty) {
			ss << " ";
		}
		ss << "PRIMARY KEY";
		empty = false;
	}
	if (field.isUnique()) {
		auto i = constraints.find(field.name);
		// only if there is one field in the unique list - otherwise we have to construct
		// them differently like the primary key for multiple fields
		if (i == constraints.end() || i->second.fields.size() == 1) {
			if (!empty) {
				ss << " ";
			}
			ss << "UNIQUE";
			empty = false;
		}
	}
	if (!field.defaultVal.empty()) {
		if (!empty) {
			ss << " ";
		}
		ss << "DEFAULT " << field.defaultVal;
		empty = false;
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
