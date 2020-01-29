/**
 * @file
 */

#pragma once

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
#include "Blob.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

namespace persistence {

extern core::String toLower(const char *in);
extern core::String toLower(const core::String& in);

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
	struct Meta {
		const char* _schema;
		const char* _tableName;
		uint8_t _primaryKeyFields = 0u;
		const char* _autoIncrementField = nullptr;
		long _autoIncrementStart = 1l;
		Fields _fields;
		Constraints _constraints;
		UniqueKeys _uniqueKeys;
		ForeignKeys _foreignKeys;
		PrimaryKeys _primaryKeys;
	};

	friend class DBHandler;
	friend class MassQuery;
	bool _flagToDelete = false;
	uint8_t* _membersPointer;
	const Meta* _s;
	/**
	 * @brief Put the current row into this model instance.
	 * @param[in,out] state The State of the query. Increases the current row for
	 * each new model that calls this
	 */
	bool fillModelValues(State& state);

public:
	Model(const Meta* s);
	virtual ~Model();

	/**
	 * @return The table name without schema
	 * @see schema()
	 */
	const char* tableName() const;

	/**
	 * @return The schema name the model is located in
	 * @see tableName()
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

	/**
	 * @return The name of the @c autoincrement field, or @c null if there is no such column.
	 */
	const char* autoIncrementField() const;

	/**
	 * @note Used in the @c MassQuery to order the model into the right list.
	 */
	bool shouldBeDeleted() const;

	/**
	 * @brief Flag for delete for the @c MassQuery and @c ISavable infrastructure
	 */
	void flagForDelete();

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
		core_assert_msg(f.nulloffset < 0, "Use getValuePointer()");
		core_assert(f.offset >= 0);
		const uint8_t* target = (const uint8_t*)(_membersPointer + f.offset);
		const T* targetValue = (const T*)target;
		return *targetValue;
	}

	template<class T>
	const T* getValuePointer(const Field& f) const {
		core_assert_msg(f.nulloffset >= 0, "Use getValue()");
		core_assert(f.offset >= 0);
		const uint8_t* target = (const uint8_t*)(_membersPointer + f.offset);
		const T* targetValue = (const T*)target;
		return targetValue;
	}

	/**
	 * @brief Set the @c Field value to null (if supported)
	 */
	void setValue(const Field& f, std::nullptr_t np);
	void setValue(const Field& f, const core::String& value);
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
	const Field& getField(const core::String& name) const;

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
	return _s->_tableName;
}

inline const char* Model::schema() const {
	return _s->_schema;
}

inline const Field& Model::getField(const core::String& name) const {
	return getField(name.c_str());
}

inline const Fields& Model::fields() const {
	return _s->_fields;
}

inline const Constraints& Model::constraints() const {
	return _s->_constraints;
}

inline const UniqueKeys& Model::uniqueKeys() const {
	return _s->_uniqueKeys;
}

inline const PrimaryKeys& Model::primaryKeys() const {
	return _s->_primaryKeys;
}

inline const ForeignKeys& Model::foreignKeys() const {
	return _s->_foreignKeys;
}

inline int Model::primaryKeyFields() const {
	return _s->_primaryKeyFields;
}

inline const char* Model::autoIncrementField() const {
	return _s->_autoIncrementField;
}

inline long Model::autoIncrementStart() const {
	return _s->_autoIncrementStart;
}

inline void Model::flagForDelete() {
	_flagToDelete = true;
}

inline bool Model::shouldBeDeleted() const {
	return _flagToDelete;
}

}
