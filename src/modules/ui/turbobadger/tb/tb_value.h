/**
 * @file
 */

#pragma once

#include "tb_core.h"
#include "tb_list.h"

namespace tb {

class TBValue;
class TBTypedObject;

/** Return true if the given string starts with a number.
	Ex: 100, -.2, 1.0E-8, 5px will all return true. */
bool is_start_of_number(const char *str);

/** Returns true if the given string contains space that
	is not at the end of the string. */
bool contains_non_trailing_space(const char *str);

/** Return true if the string can be represented as a number.
	It ignores trailing white space.
	Ex: 100, -.2 will return true.
	Ex: 1.0E-8, 5px will return false. */
bool is_number_only(const char *str);

/** Return true if the given number string is a float number.
	Should only be called when you've verified it's a number with is_number(). */
bool is_number_float(const char *str);

/** TBValueArray is an array of TBValue */
class TBValueArray {
public:
	TBValueArray();
	~TBValueArray();
	TBValue *addValue();
	TBValue *getValue(int index);
	static TBValueArray *clone(TBValueArray *source);
	int getLength() const {
		return m_list.getNumItems();
	}

private:
	TBListAutoDeleteOf<TBValue> m_list;
};

/** TBValue holds value of a specific type.
	In addition to NULL, string, float, integer, it may also contain an array
	of attributes (TBValueArray), or an object (derived from TBTypedObject).

	When getting the value as a different type from what it is, it may convert
	its internal representation to that type. Exceptions are for array and
	object, which will return 0 when getting as numbers, or "" or object name
	when getting as string. */
class TBValue {
public:
	/** The current type of the value.
		It may change when using a getter of a different type. */
	enum TYPE { TYPE_NULL, TYPE_STRING, TYPE_FLOAT, TYPE_INT, TYPE_OBJECT, TYPE_ARRAY };

	/** How to deal with the dynamic memory when setting string and array. */
	enum SET {
		SET_NEW_COPY,		///< A new copy of the data will be made.
		SET_TAKE_OWNERSHIP, ///< The data passed in will be stored and freed.
		SET_AS_STATIC		///< The data passed in will be stored but never freed.
	};

	TBValue();
	TBValue(const TBValue &value);
	TBValue(TYPE type);

	TBValue(int value);
	TBValue(float value);
	TBValue(const char *value, SET set = SET_NEW_COPY);
	TBValue(TBTypedObject *object);

	~TBValue();

	/** Take over ownership of content of source_value.
		Note:	-If source_value has string or array that are set with SET_AS_STATIC, it will make new copies of those.
				-value will be nulled on source_value after this call. */
	void takeOver(TBValue &source_value);

	/** Copy the content of source_value to this value.
		Note: This value will become TYPE_NULL if source_value holds an owned object. We can't copy objects. */
	void copy(const TBValue &source_value);

	void setNull();
	void setInt(int val);
	void setFloat(float val);

	/** Set the passed in string */
	void setString(const char *val, SET set);

	/** Set the passed in object. Takes the ownership of the object! */
	void setObject(TBTypedObject *object);

	/** Set the passed in array */
	void setArray(TBValueArray *arr, SET set);

	/** Set the value either as a string, number or array of numbers, depending of the string syntax. */
	void setFromStringAuto(const char *str, SET set);

	int getInt() const;
	float getFloat() const;
	const char *getString();
	TBTypedObject *getObject() const {
		return isObject() ? val_obj : nullptr;
	}
	TBValueArray *getArray() const {
		return isArray() ? val_arr : nullptr;
	}

	TYPE getType() const {
		return (TYPE)m_packed.type;
	}
	bool isNull() const {
		return m_packed.type == TYPE_NULL;
	}
	bool isString() const {
		return m_packed.type == TYPE_STRING;
	}
	bool isFloat() const {
		return m_packed.type == TYPE_FLOAT;
	}
	bool isInt() const {
		return m_packed.type == TYPE_INT;
	}
	bool isObject() const {
		return m_packed.type == TYPE_OBJECT;
	}
	bool isArray() const {
		return m_packed.type == TYPE_ARRAY;
	}
	int getArrayLength() const {
		return isArray() ? val_arr->getLength() : 0;
	}

	const TBValue &operator=(const TBValue &val) {
		copy(val);
		return *this;
	}

private:
	union {
		float val_float;
		int val_int;
		char *val_str;
		TBTypedObject *val_obj;
		TBValueArray *val_arr;
	};
	union {
		struct {
			uint32_t type : 8;
			uint32_t allocated : 1;
		} m_packed;
		uint32_t m_packed_init;
	};
};

} // namespace tb
