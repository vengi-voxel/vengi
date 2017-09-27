/**
 * @file
 */

#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <unordered_map>
#include <vector>
#include <set>
#include "core/Common.h"
#include "Timestamp.h"
#include "FieldType.h"
#include "PreparedStatement.h"
#include "State.h"
#include "ConstraintType.h"
#include "config.h"

#include <libpq-fe.h>
using ResultType = PGresult;

namespace persistence {

struct Field {
	std::string name;
	FieldType type = FieldType::STRING;
	// bitmask from ConstraintType
	uint32_t contraintMask = 0u;
	std::string defaultVal = "";
	int length = 0;
	intptr_t offset = -1;
	intptr_t nulloffset = -1;

	inline bool isAutoincrement() const {
		return (contraintMask & std::enum_value(ConstraintType::AUTOINCREMENT)) != 0u;
	}

	inline bool isNotNull() const {
		return (contraintMask & std::enum_value(ConstraintType::NOTNULL)) != 0u;
	}

	inline bool isPrimaryKey() const {
		return (contraintMask & std::enum_value(ConstraintType::PRIMARYKEY)) != 0u;
	}

	inline bool isUnique() const {
		return (contraintMask & std::enum_value(ConstraintType::UNIQUE)) != 0u;
	}
};
typedef std::vector<Field> Fields;

struct Constraint {
	std::vector<std::string> fields;
	// bitmask from persistence::Model::ConstraintType
	uint32_t types;
};
typedef std::unordered_map<std::string, Constraint> Constraints;
typedef std::vector<std::set<std::string>> UniqueKeys;

class DBHandler;
class Connection;

class Model {
protected:
	friend class DBHandler;
	friend class PreparedStatement;
	Fields _fields;
	const std::string _tableName;
	int _primaryKeys = 0;
	uint8_t* _membersPointer;
	Constraints _constraints;
	UniqueKeys _uniqueKeys;

	const Field& getField(const std::string& name) const;
	bool checkLastResult(State& state, Connection* connection) const;
	bool fillModelValues(State& state);
public:
	Model(const std::string& tableName);
	virtual ~Model();

	const std::string& tableName() const;

	const Fields& fields() const;

	const Constraints& constraints() const;

	const UniqueKeys& uniqueKeys() const;

	int primaryKeys() const;

	bool isPrimaryKey(const std::string& fieldname) const;

	bool begin();
	bool commit();
	bool rollback();

	void setValue(const Field& f, const std::string& value) {
		core_assert(f.offset >= 0);
		uint8_t* target = (uint8_t*)(_membersPointer + f.offset);
		std::string* targetValue = (std::string*)target;
		*targetValue = value;
	}

	void setValue(const Field& f, const Timestamp& value) {
		core_assert(f.offset >= 0);
		uint8_t* target = (uint8_t*)(_membersPointer + f.offset);
		Timestamp* targetValue = (Timestamp*)target;
		*targetValue = value;
	}

	template<class TYPE>
	void setValue(const Field& f, const TYPE& value) {
		core_assert(f.offset >= 0);
		uint8_t* target = (uint8_t*)(_membersPointer + f.offset);
		TYPE* targetValue = (TYPE*)target;
		*targetValue = value;
	}

	inline void setIsNull(const Field& f, bool isNull) {
		if (f.nulloffset == -1) {
			return;
		}
		uint8_t* target = (uint8_t*)(_membersPointer + f.nulloffset);
		bool* targetValue = (bool*)target;
		*targetValue = isNull;
	}

	PreparedStatement prepare(const std::string& name, const std::string& statement);

	bool exec(const std::string& query) const;
	bool exec(const std::string& query);

	bool exec(const char* query) const;
	bool exec(const char* query);
};

inline bool Model::exec(const std::string& query) const {
	return exec(query.c_str());
}

inline bool Model::exec(const std::string& query) {
	return exec(query.c_str());
}

inline const std::string& Model::tableName() const {
	return _tableName;
}

inline const Fields& Model::fields() const {
	return _fields;
}

inline const Constraints& Model::constraints() const {
	return _constraints;
}

inline const UniqueKeys& Model::uniqueKeys() const {
	return _uniqueKeys;
}

inline int Model::primaryKeys() const {
	return _primaryKeys;
}

}
