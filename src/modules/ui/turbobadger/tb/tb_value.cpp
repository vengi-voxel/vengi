/**
 * @file
 */

#include "tb_value.h"
#include "core/Assert.h"
#include "core/StringUtil.h"
#include "tb_object.h"
#include "tb_str.h"
#include <SDL_stdinc.h>

namespace tb {

// FIX: ## Floating point string conversions might be locale dependant. Force "." as decimal!

char *next_token(char *&str, const char *delim) {
	str += strspn(str, delim);
	if (*str == 0) {
		return nullptr;
	}
	char *token = str;
	str += strcspn(str, delim);
	if (*str != 0) {
		*str++ = '\0';
	}
	return token;
}

bool is_start_of_number(const char *str) {
	if (*str == '-') {
		str++;
	}
	if (*str == '.') {
		str++;
	}
	return *str >= '0' && *str <= '9';
}

bool contains_non_trailing_space(const char *str) {
	if (const char *p = SDL_strstr(str, " ")) {
		while (*p == ' ') {
			p++;
		}
		return *p != '\0';
	}
	return false;
}

bool is_number_only(const char *s) {
	if ((s == nullptr) || *s == 0 || *s == ' ') {
		return false;
	}
	char *p;
	SDL_strtod(s, &p);
	while (*p == ' ') {
		p++;
	}
	return *p == '\0';
}

bool is_number_float(const char *str) {
	while (*str != 0) {
		if (*str++ == '.') {
			return true;
		}
	}
	return false;
}

TBValueArray::TBValueArray() {
}

TBValueArray::~TBValueArray() {
}

TBValue *TBValueArray::addValue() {
	TBValue *v;
	if (((v = new TBValue()) != nullptr) && m_list.add(v)) {
		return v;
	}
	delete v;
	return nullptr;
}

TBValue *TBValueArray::getValue(int index) {
	if (index >= 0 && index < m_list.getNumItems()) {
		return m_list[index];
	}
	return nullptr;
}

TBValueArray *TBValueArray::clone(TBValueArray *source) {
	TBValueArray *new_arr = new TBValueArray;
	if (new_arr == nullptr) {
		return nullptr;
	}
	for (int i = 0; i < source->m_list.getNumItems(); i++) {
		TBValue *new_val = new_arr->addValue();
		if (new_val == nullptr) {
			delete new_arr;
			return nullptr;
		}
		new_val->copy(*source->getValue(i));
	}
	return new_arr;
}

TBValue::TBValue() : m_packed_init(0) {
}

TBValue::TBValue(const TBValue &value) : m_packed_init(0) {
	copy(value);
}

TBValue::TBValue(TYPE type) : m_packed_init(0) {
	switch (type) {
	case TYPE_NULL:
		setNull();
		break;
	case TYPE_STRING:
		setString("", SET_AS_STATIC);
		break;
	case TYPE_FLOAT:
		setFloat(0);
		break;
	case TYPE_INT:
		setInt(0);
		break;
	case TYPE_OBJECT:
		setObject(nullptr);
		break;
	case TYPE_ARRAY:
		if (TBValueArray *arr = new TBValueArray()) {
			setArray(arr, SET_TAKE_OWNERSHIP);
		}
		break;
	default:
		core_assert(!"Not implemented!");
	}
}

TBValue::TBValue(int value) : m_packed_init(0) {
	setInt(value);
}

TBValue::TBValue(float value) : m_packed_init(0) {
	setFloat(value);
}

TBValue::TBValue(const char *value, SET set) : m_packed_init(0) {
	setString(value, set);
}

TBValue::TBValue(TBTypedObject *object) : m_packed_init(0) {
	setObject(object);
}

TBValue::~TBValue() {
	setNull();
}

void TBValue::takeOver(TBValue &sourceValue) {
	if (sourceValue.m_packed.type == TYPE_STRING) {
		setString(sourceValue.val_str, sourceValue.m_packed.allocated ? SET_TAKE_OWNERSHIP : SET_NEW_COPY);
	} else if (sourceValue.m_packed.type == TYPE_ARRAY) {
		setArray(sourceValue.val_arr, sourceValue.m_packed.allocated ? SET_TAKE_OWNERSHIP : SET_NEW_COPY);
	} else {
		*this = sourceValue;
	}
	sourceValue.m_packed.type = TYPE_NULL;
}

void TBValue::copy(const TBValue &sourceValue) {
	if (sourceValue.m_packed.type == TYPE_STRING) {
		setString(sourceValue.val_str, SET_NEW_COPY);
	} else if (sourceValue.m_packed.type == TYPE_ARRAY) {
		setArray(sourceValue.val_arr, SET_NEW_COPY);
	} else if (sourceValue.m_packed.type == TYPE_OBJECT) {
		core_assert(!"We can't copy objects! The value will be nulled!");
		setObject(nullptr);
	} else {
		setNull();
		this->m_packed_init = sourceValue.m_packed_init;
		this->val_obj = sourceValue.val_obj;
	}
}

void TBValue::setNull() {
	if (m_packed.allocated) {
		if (m_packed.type == TYPE_STRING) {
			SDL_free(val_str);
		} else if (m_packed.type == TYPE_OBJECT) {
			delete val_obj;
		} else if (m_packed.type == TYPE_ARRAY) {
			delete val_arr;
		}
	}
	m_packed.type = TYPE_NULL;
}

void TBValue::setInt(int val) {
	setNull();
	m_packed.type = TYPE_INT;
	val_int = val;
}

void TBValue::setFloat(float val) {
	setNull();
	m_packed.type = TYPE_FLOAT;
	val_float = val;
}

void TBValue::setString(const char *val, SET set) {
	setNull();
	m_packed.allocated = (set == SET_NEW_COPY || set == SET_TAKE_OWNERSHIP);
	if (set != SET_NEW_COPY) {
		val_str = const_cast<char *>(val);
		m_packed.type = TYPE_STRING;
	} else if ((val_str = SDL_strdup(val)) != nullptr) {
		m_packed.type = TYPE_STRING;
	}
}

void TBValue::setObject(TBTypedObject *object) {
	setNull();
	m_packed.type = TYPE_OBJECT;
	m_packed.allocated = true;
	val_obj = object;
}

void TBValue::setArray(TBValueArray *arr, SET set) {
	setNull();
	m_packed.allocated = (set == SET_NEW_COPY || set == SET_TAKE_OWNERSHIP);
	if (set != SET_NEW_COPY) {
		val_arr = arr;
		m_packed.type = TYPE_ARRAY;
	} else if ((val_arr = TBValueArray::clone(arr)) != nullptr) {
		m_packed.type = TYPE_ARRAY;
	}
}

void TBValue::setFromStringAuto(const char *str, SET set) {
	if (str == nullptr) {
		setNull();
	} else if (is_number_only(str)) {
		if (is_number_float(str)) {
			setFloat(core::string::toFloat(str));
		} else {
			setInt(core::string::toInt(str));
		}
	} else if (is_start_of_number(str) && contains_non_trailing_space(str)) {
		// If the number has nontrailing space, we'll assume a list of numbers (example: "10 -4 3.5")
		setNull();
		if (TBValueArray *arr = new TBValueArray) {
			core::String tmpstr = str;
			char *str_next = tmpstr.c_str();
			while (char *token = next_token(str_next, ", ")) {
				if (TBValue *new_val = arr->addValue()) {
					new_val->setFromStringAuto(token, SET_NEW_COPY);
				}
			}
			setArray(arr, SET_TAKE_OWNERSHIP);
		}
	} else if (*str == '[') {
		setNull();
		if (TBValueArray *arr = new TBValueArray) {
			core_assert(!"not implemented! Split out the tokenizer code above!");
			setArray(arr, SET_TAKE_OWNERSHIP);
		}
	} else {
		setString(str, set);
		return;
	}
	// We didn't set as string, so we might need to deal with the passed in string data.
	if (set == SET_TAKE_OWNERSHIP) {
		// Delete the passed in data
		TBValue tmp;
		tmp.setString(str, SET_TAKE_OWNERSHIP);
	}
}

int TBValue::getInt() const {
	if (m_packed.type == TYPE_STRING) {
		return core::string::toInt(val_str);
	}
	if (m_packed.type == TYPE_FLOAT) {
		return (int)val_float;
	}
	return m_packed.type == TYPE_INT ? val_int : 0;
}

float TBValue::getFloat() const {
	if (m_packed.type == TYPE_STRING) {
		return core::string::toFloat(val_str);
	}
	if (m_packed.type == TYPE_INT) {
		return (float)val_int;
	}
	return m_packed.type == TYPE_FLOAT ? val_float : 0;
}

const char *TBValue::getString() {
	if (m_packed.type == TYPE_INT) {
		char tmp[32];
		SDL_snprintf(tmp, sizeof(tmp), "%d", val_int);
		setString(tmp, SET_NEW_COPY);
	} else if (m_packed.type == TYPE_FLOAT) {
		char tmp[32];
		SDL_snprintf(tmp, sizeof(tmp), "%f", val_float);
		setString(tmp, SET_NEW_COPY);
	} else if (m_packed.type == TYPE_OBJECT) {
		return val_obj != nullptr ? val_obj->getClassName() : "";
	}
	return m_packed.type == TYPE_STRING ? val_str : "";
}

} // namespace tb
