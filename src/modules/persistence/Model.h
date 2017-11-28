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

	/**
	 * @return The schema name the model is located in
	 */
	const std::string& schema() const;

	/**
	 * @return Collection of all @c Field instances that define this model
	 */
	const Fields& fields() const;

	/**
	 * @return Collection of all @c Constraint instances
	 */
	const Constraints& constraints() const;

	/**
	 * @return Collection of all unique keys. Defines by the set of names of the participating @c Field instances
	 */
	const UniqueKeys& uniqueKeys() const;

	/**
	 * @return Collection of foreign keys.
	 */
	const ForeignKeys& foreignKeys() const;

	/**
	 * @return The value to start the model auto increment sequence with. This is 1 by default if not specified otherwise.
	 */
	long autoIncrementStart() const;

	/**
	 * @return The number of primary keys that this model contains
	 */
	int primaryKeys() const;

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

	/**
	 * @return @c Field instance for the given colume name
	 */
	const Field& getField(const std::string& name) const;

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

inline const std::string& Model::tableName() const {
	return _tableName;
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
