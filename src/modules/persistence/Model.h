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

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

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
	friend class MassQuery;
	const char* _schema;
	const char* _tableName;
	int _primaryKeyFields = 0;
	const char* _autoIncrementField = nullptr;
	long _autoIncrementStart = 1l;
	uint8_t* _membersPointer;
	const FieldsPtr _fields;
	const ConstraintsPtr _constraints;
	const UniqueKeysPtr _uniqueKeys;
	const ForeignKeysPtr _foreignKeys;
	const PrimaryKeysPtr _primaryKeys;

	bool fillModelValues(State& state);
public:
	Model(const char* schema, const char*tableName, const FieldsPtr fields, const ConstraintsPtr constraints,
			const UniqueKeysPtr uniqueKeys, const ForeignKeysPtr foreignKeys, const PrimaryKeysPtr primaryKeys);
	virtual ~Model();

	const char* tableName() const;

	/**
	 * @return The schema name the model is located in
	 */
	const char* schema() const;

	/**
	 * @return Collection of all @c Field instances that define this model
	 */
	const Fields& fields() const;

	/**
	 * @return Collection of all @c Constraint instances
	 */
	const Constraints& constraints() const;

	/**
	 * @return Collection of all primary key fields.
	 */
	const PrimaryKeys& primaryKeys() const;

	/**
	 * @return Collection of all unique keys. Defined by the set of names of the participating @c Field instances
	 */
	const UniqueKeys& uniqueKeys() const;

	/**
	 * @return Collection of foreign keys.
	 */
	const ForeignKeys& foreignKeys() const;

	const char* autoIncrementField() const;

	/**
	 * @return The value to start the model auto increment sequence with. This is 1 by default if not specified otherwise.
	 */
	long autoIncrementStart() const;

	/**
	 * @return The number of primary key fields that this model contains
	 */
	int primaryKeyFields() const;

	/**
	 * @return @c true if the field was set to a valid value (which might also be null)
	 */
	bool isValid(const Field& f) const;
	/**
	 * @see isValid()
	 * @see setValid()
	 */
	void reset(const Field& f);
	/**
	 * @see isValid()
	 * @see reset()
	 */
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

	/**
	 * @brief Set the @c Field value to null (if supported)
	 */
	void setValue(const Field& f, std::nullptr_t np);
	void setValue(const Field& f, const std::string& value);
	void setValue(const Field& f, const Timestamp& value);

	template<class TYPE>
	void setValue(const Field& f, const TYPE& value) {
		core_assert(f.offset >= 0);
		uint8_t* target = (uint8_t*)(_membersPointer + f.offset);
		TYPE* targetValue = (TYPE*)target;
		*targetValue = value;
	}

	/**
	 * @return @c Field instance for the given colume name
	 */
	const Field& getField(const std::string& name) const;

	const Field& getField(const char* name) const;

	/**
	 * @brief Indicate that the value of the field should be written as null to the database
	 * @note This only works if the @c Field instance can be set to null. If the @c Field is defined
	 * as non-null. This is just a nop.
	 */
	void setIsNull(const Field& f, bool isNull);
	/**
	 * @return @c true if the given @c Field is currently set to null. @c false if not, or can't be null at all.
	 */
	bool isNull(const Field& f) const;
};

inline const char* Model::tableName() const {
	return _tableName;
}

inline const char* Model::schema() const {
	return _schema;
}

inline const Field& Model::getField(const std::string& name) const {
	return getField(name.c_str());
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

inline const PrimaryKeys& Model::primaryKeys() const {
	return *_primaryKeys;
}

inline const ForeignKeys& Model::foreignKeys() const {
	return *_foreignKeys;
}

inline int Model::primaryKeyFields() const {
	return _primaryKeyFields;
}

inline const char* Model::autoIncrementField() const {
	return _autoIncrementField;
}

inline long Model::autoIncrementStart() const {
	return _autoIncrementStart;
}

}
