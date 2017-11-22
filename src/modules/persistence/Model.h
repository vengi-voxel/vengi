/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/Assert.h"
#include "Timestamp.h"
#include "FieldType.h"
#include "BindParam.h"
#include "State.h"
#include "ConstraintType.h"
#include "ForwardDecl.h"
#include "Order.h"
#include "Field.h"
#include "Structs.h"

#include <cstdio>
#include <cstdlib>
#include <cstddef>

namespace persistence {

typedef std::vector<Field> Fields;
using FieldName = const char *;
typedef Fields* FieldsPtr;

/**
 * @brief The base class for your database models.
 *
 * Contains metadata to build the needed sql statements in the @c DBHandler
 *
 * @ingroup Persistence
 */
class Model {
protected:
	friend class DBHandler;
	friend class PreparedStatement;
	std::string _schema;
	std::string _tableName;
	std::string _fullTableName;
	int _primaryKeys = 0;
	long _autoIncrementStart = 1l;
	uint8_t* _membersPointer;
	const FieldsPtr _fields;
	const ConstraintsPtr _constraints;
	const UniqueKeysPtr _uniqueKeys;
	const ForeignKeysPtr _foreignKeys;

	bool fillModelValues(State& state);
public:
	Model(const std::string& schema, const std::string& tableName, const FieldsPtr fields, const ConstraintsPtr constraints,
			const UniqueKeysPtr uniqueKeys, const ForeignKeysPtr foreignKeys);
	virtual ~Model();

	const std::string& tableName() const;

	const std::string& fullTableName() const;

	const std::string& schema() const;

	const Fields& fields() const;

	const Constraints& constraints() const;

	const UniqueKeys& uniqueKeys() const;

	const ForeignKeys& foreignKeys() const;

	long autoIncrementStart() const;

	int primaryKeys() const;

	bool isPrimaryKey(const std::string& fieldname) const;

	/**
	 * @return @c true if the field was set to a valid value (which might also be null)
	 */
	bool isValid(const Field& f) const;
	void reset(const Field& f);
	void setValid(const Field& f, bool valid);

	template<class T>
	T getValue(const Field& f) const {
		core_assert(f.nulloffset < 0);
		core_assert(f.offset >= 0);
		const uint8_t* target = (const uint8_t*)(_membersPointer + f.offset);
		const T* targetValue = (const T*)target;
		return *targetValue;
	}

	template<class T>
	const T* getValuePointer(const Field& f) const {
		core_assert(f.nulloffset >= 0);
		core_assert(f.offset >= 0);
		const uint8_t* target = (const uint8_t*)(_membersPointer + f.offset);
		const T* targetValue = (const T*)target;
		return targetValue;
	}

	void setValue(const Field& f, nullptr_t np);
	void setValue(const Field& f, const std::string& value);
	void setValue(const Field& f, const Timestamp& value);

	template<class TYPE>
	void setValue(const Field& f, const TYPE& value) {
		core_assert(f.offset >= 0);
		uint8_t* target = (uint8_t*)(_membersPointer + f.offset);
		TYPE* targetValue = (TYPE*)target;
		*targetValue = value;
	}

	const Field& getField(const std::string& name) const;

	void setIsNull(const Field& f, bool isNull);
	bool isNull(const Field& f) const;

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

inline const std::string& Model::fullTableName() const {
	return _fullTableName;
}

inline const std::string& Model::schema() const {
	return _schema;
}

inline const Fields& Model::fields() const {
	return *_fields;
}

inline const Constraints& Model::constraints() const {
	return *_constraints;
}

inline const UniqueKeys& Model::uniqueKeys() const {
	return *_uniqueKeys;
}

inline const ForeignKeys& Model::foreignKeys() const {
	return *_foreignKeys;
}

inline int Model::primaryKeys() const {
	return _primaryKeys;
}

inline long Model::autoIncrementStart() const {
	return _autoIncrementStart;
}

}
