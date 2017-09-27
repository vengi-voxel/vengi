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

class DBHandler;
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
		TEXT,
		LONG,
		INT,
		PASSWORD,
		TIMESTAMP,
		MAX
	};
	static constexpr int MAX_FIELDTYPES = std::enum_value(FieldType::MAX);

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
	typedef std::unordered_map<std::string, persistence::Model::Constraint> Constraints;

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
		int currentRow = -1;
		// false on error, true on success
		bool result = false;
	};
protected:
	Fields _fields;
	const std::string _tableName;
	uint8_t* _membersPointer;

	const Field& getField(const std::string& name) const;
	bool checkLastResult(State& state, Connection* connection) const;
	bool fillModelValues(State& state);
public:
	Model(const std::string& tableName);
	virtual ~Model();

	const std::string& tableName() const;

	const Fields& fields() const;

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
		struct BindParam {
			std::vector<const char *> values;
			std::vector<int> lengths;
			std::vector<int> formats;
			std::vector<std::string> valueBuffers;
			std::vector<FieldType> fieldTypes;
			int position = 0;
			BindParam(int num) :
					values(num, nullptr), lengths(num, 0), formats(num, 0), fieldTypes(num, FieldType::INT) {
				valueBuffers.reserve(num);
			}

			int add() {
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
		};
		BindParam _params;
	public:
		PreparedStatement(Model* model, const std::string& name, const std::string& statement);

		PreparedStatement& add(const std::string& value, FieldType fieldType) {
			const int index = _params.add();
			_params.valueBuffers.emplace_back(value);
			_params.fieldTypes[index] = fieldType;
			_params.values[index] = _params.valueBuffers.back().data();
			return *this;
		}

		PreparedStatement& add(const std::string& value) {
			return add(value, FieldType::STRING);
		}

		PreparedStatement& add(int value) {
			return add(std::to_string(value), FieldType::INT);
		}

		PreparedStatement& add(long value) {
			return add(std::to_string(value), FieldType::LONG);
		}

		PreparedStatement& addPassword(const std::string& password) {
			return add(password, FieldType::PASSWORD);
		}

		PreparedStatement& addPassword(const char* password) {
			const int index = _params.add();
			_params.fieldTypes[index] = FieldType::PASSWORD;
			_params.values[index] = password;
			_params.lengths[index] = strlen(password);
			return *this;
		}

		PreparedStatement& add(const char* value, FieldType fieldType) {
			const int index = _params.add();
			_params.fieldTypes[index] = fieldType;
			_params.values[index] = value;
			_params.lengths[index] = strlen(value);
			return *this;
		}

		PreparedStatement& add(nullptr_t value, FieldType fieldType) {
			const int index = _params.add();
			_params.fieldTypes[index] = fieldType;
			_params.values[index] = value;
			return *this;
		}

		PreparedStatement& add(const Timestamp& type) {
			if (type.isNow()) {
				return add("NOW()", FieldType::TIMESTAMP);
			}
			return add(std::to_string(type.time()), FieldType::TIMESTAMP);
		}

		State exec();
	};

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

inline const Model::Fields& Model::fields() const {
	return _fields;
}

}
