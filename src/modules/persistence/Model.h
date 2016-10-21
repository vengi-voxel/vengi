/**
 * @file
 */

#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <unordered_map>
#include <vector>
#include "core/Common.h"
#include "Timestamp.h"
#include "config.h"

#include <libpq-fe.h>
using ResultType = PGresult;

namespace persistence {

class Connection;

class Model {
public:
	// don't change the order - code generator relies on this
	enum class ConstraintType {
		UNIQUE = 1 << 0,
		PRIMARYKEY = 1 << 1,
		AUTOINCREMENT = 1 << 2,
		NOTNULL = 1 << 3
	};
	static constexpr int MAX_CONSTRAINTTYPES = 4;

	// don't change the order - code generator relies on this
	enum class FieldType {
		STRING,
		LONG,
		INT,
		PASSWORD,
		TIMESTAMP
	};
	static constexpr int MAX_FIELDTYPES = 5;

	struct Field {
		std::string name;
		FieldType type = FieldType::STRING;
		// bitmask from ConstraintType
		uint32_t contraintMask = 0u;
		std::string defaultVal = "";
		int length = 0;
		intptr_t offset = 0;

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

	class State {
	public:
		State(ResultType* res);
		~State();

		State(State&& other);

		State(const State& other) = delete;
		State& operator=(const State& other) = delete;

		ResultType* res = nullptr;

		std::string lastErrorMsg;
		int affectedRows = -1;
		// false on error, true on success
		bool result = false;
	};
protected:
	Fields _fields;
	const std::string _tableName;
	uint8_t* _membersPointer;

	Field getField(const std::string& name) const;
	bool checkLastResult(State& state, Connection* connection) const;
	bool fillModelValues(State& state);
public:
	Model(const std::string& tableName);
	virtual ~Model();

	const std::string& getTableName() const;

	const Fields& getFields() const;

	bool isPrimaryKey(const std::string& fieldname) const;

	bool begin();
	bool commit();
	bool rollback();

	class ScopedTransaction {
	private:
		bool _commited = false;
		bool _autocommit;
		Model* _model;
	public:
		ScopedTransaction(Model* model, bool autocommit = true);
		~ScopedTransaction();

		void commit();
		void rollback();
	};

	class PreparedStatement {
	private:
		Model* _model;
		std::string _name;
		std::string _statement;
		typedef std::pair<std::string, FieldType> ParamEntry;
		std::vector<ParamEntry> _params;
	public:
		PreparedStatement(Model* model, const std::string& name, const std::string& statement);

		inline PreparedStatement& add(const std::string& type, FieldType fieldType) {
			_params.push_back(std::make_pair(type, fieldType));
			return *this;
		}

		template<class Type>
		PreparedStatement& add(const Type& type, FieldType fieldType) {
			return add(std::to_string(type), fieldType);
		}

		inline PreparedStatement& add(const std::string& type) {
			return add(type, FieldType::STRING);
		}

		inline PreparedStatement& addPassword(const std::string& password) {
			return add(password, FieldType::PASSWORD);
		}

		inline PreparedStatement& add(const char* type, FieldType fieldType) {
			return add(std::string(type), FieldType::STRING);
		}

		inline PreparedStatement& add(const Timestamp& type) {
			if (type.isNow()) {
				return add("NOW()", FieldType::TIMESTAMP);
			}
			return add(type.time(), FieldType::TIMESTAMP);
		}

		State exec();
	};

	template<class TYPE>
	inline void setValue(const Field& f, const TYPE& value) {
		uint8_t* target = (uint8_t*)(_membersPointer + f.offset);
		TYPE* targetValue = (TYPE*)target;
		*targetValue = value;
	}

	PreparedStatement prepare(const std::string& name, const std::string& statement);

	bool exec(const std::string& query);

	bool exec(const char* query);
};

inline bool Model::exec(const std::string& query) {
	return exec(query.c_str());
}

inline const std::string& Model::getTableName() const {
	return _tableName;
}

inline const Model::Fields& Model::getFields() const {
	return _fields;
}

}
