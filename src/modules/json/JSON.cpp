/**
 * @file
 */

#include "JSON.h"
#include <cJSON.h>

namespace json {

Json::Json() : _json(nullptr), _owned(false) {
}

Json::~Json() {
	if (_owned && _json) {
		cJSON_Delete(_json);
	}
}

Json::Json(const Json &other) : _owned(other._owned) {
	if (other._owned && other._json) {
		_json = cJSON_Duplicate(other._json, cJSON_True);
	} else {
		_json = other._json;
	}
}

Json::Json(Json &&other) noexcept : _json(other._json), _owned(other._owned) {
	other._json = nullptr;
	other._owned = false;
}

Json &Json::operator=(const Json &other) {
	if (this == &other) {
		return *this;
	}
	if (_owned && _json) {
		cJSON_Delete(_json);
	}
	_owned = other._owned;
	if (other._owned && other._json) {
		_json = cJSON_Duplicate(other._json, cJSON_True);
	} else {
		_json = other._json;
	}
	return *this;
}

Json &Json::operator=(Json &&other) noexcept {
	if (this == &other) {
		return *this;
	}
	if (_owned && _json) {
		cJSON_Delete(_json);
	}
	_json = other._json;
	_owned = other._owned;
	other._json = nullptr;
	other._owned = false;
	return *this;
}

Json::Json(cJSON *json, bool owned) : _json(json), _owned(owned) {
}

Json Json::parse(const char *str) {
	if (str == nullptr) {
		return Json();
	}
	cJSON *parsed = cJSON_Parse(str);
	return Json(parsed, true);
}

Json Json::parse(const core::String &str) {
	return parse(str.c_str());
}

bool Json::accept(const char *str) {
	if (str == nullptr) {
		return false;
	}
	cJSON *parsed = cJSON_Parse(str);
	if (parsed == nullptr) {
		return false;
	}
	cJSON_Delete(parsed);
	return true;
}

Json Json::object() {
	return Json(cJSON_CreateObject(), true);
}

Json Json::array() {
	return Json(cJSON_CreateArray(), true);
}

bool Json::isValid() const {
	return _json != nullptr;
}

bool Json::isNull() const {
	return _json == nullptr || cJSON_IsNull(_json);
}

bool Json::isObject() const {
	return _json != nullptr && cJSON_IsObject(_json);
}

bool Json::isArray() const {
	return _json != nullptr && cJSON_IsArray(_json);
}

bool Json::isString() const {
	return _json != nullptr && cJSON_IsString(_json);
}

bool Json::isBool() const {
	return _json != nullptr && (cJSON_IsBool(_json));
}

bool Json::isNumber() const {
	return _json != nullptr && cJSON_IsNumber(_json);
}

bool Json::isNumberInteger() const {
	if (!isNumber()) {
		return false;
	}
	double v = _json->valuedouble;
	return v == (double)(int)v;
}

bool Json::isNumberFloat() const {
	if (!isNumber()) {
		return false;
	}
	double v = _json->valuedouble;
	return v != (double)(int)v;
}

bool Json::contains(const char *key) const {
	if (_json == nullptr) {
		return false;
	}
	return cJSON_HasObjectItem(_json, key);
}

Json Json::get(const char *key) const {
	if (_json == nullptr) {
		return Json();
	}
	cJSON *item = cJSON_GetObjectItemCaseSensitive(_json, key);
	return Json(item, false);
}

Json Json::get(int index) const {
	if (_json == nullptr) {
		return Json();
	}
	cJSON *item = cJSON_GetArrayItem(_json, index);
	return Json(item, false);
}

core::String Json::str() const {
	if (_json == nullptr || !cJSON_IsString(_json) || _json->valuestring == nullptr) {
		return "";
	}
	return core::String(_json->valuestring);
}

const char *Json::cStr() const {
	if (_json == nullptr || !cJSON_IsString(_json) || _json->valuestring == nullptr) {
		return "";
	}
	return _json->valuestring;
}

int Json::intVal() const {
	if (_json == nullptr || !cJSON_IsNumber(_json)) {
		return 0;
	}
	return _json->valueint;
}

double Json::doubleVal() const {
	if (_json == nullptr || !cJSON_IsNumber(_json)) {
		return 0.0;
	}
	return _json->valuedouble;
}

float Json::floatVal() const {
	return (float)doubleVal();
}

bool Json::boolVal() const {
	if (_json == nullptr) {
		return false;
	}
	return cJSON_IsTrue(_json);
}

core::String Json::strVal(const char *key, const char *defaultVal) const {
	if (_json == nullptr) {
		return defaultVal;
	}
	cJSON *item = cJSON_GetObjectItemCaseSensitive(_json, key);
	if (item == nullptr || !cJSON_IsString(item) || item->valuestring == nullptr) {
		return defaultVal;
	}
	return core::String(item->valuestring);
}

int Json::intVal(const char *key, int defaultVal) const {
	if (_json == nullptr) {
		return defaultVal;
	}
	cJSON *item = cJSON_GetObjectItemCaseSensitive(_json, key);
	if (item == nullptr || !cJSON_IsNumber(item)) {
		return defaultVal;
	}
	return item->valueint;
}

double Json::doubleVal(const char *key, double defaultVal) const {
	if (_json == nullptr) {
		return defaultVal;
	}
	cJSON *item = cJSON_GetObjectItemCaseSensitive(_json, key);
	if (item == nullptr || !cJSON_IsNumber(item)) {
		return defaultVal;
	}
	return item->valuedouble;
}

float Json::floatVal(const char *key, float defaultVal) const {
	return (float)doubleVal(key, (double)defaultVal);
}

bool Json::boolVal(const char *key, bool defaultVal) const {
	if (_json == nullptr) {
		return defaultVal;
	}
	cJSON *item = cJSON_GetObjectItemCaseSensitive(_json, key);
	if (item == nullptr || !cJSON_IsBool(item)) {
		return defaultVal;
	}
	return cJSON_IsTrue(item);
}

// Helper to add or replace a key in an object
static void addOrReplace(cJSON *obj, const char *key, cJSON *item) {
	if (cJSON_HasObjectItem(obj, key)) {
		cJSON_ReplaceItemInObjectCaseSensitive(obj, key, item);
	} else {
		cJSON_AddItemToObject(obj, key, item);
	}
}

void Json::set(const char *key, const Json &val) {
	if (_json == nullptr) {
		return;
	}
	cJSON *dup = val._json ? cJSON_Duplicate(val._json, cJSON_True) : cJSON_CreateNull();
	addOrReplace(_json, key, dup);
}

void Json::set(const char *key, const char *val) {
	if (_json == nullptr) {
		return;
	}
	addOrReplace(_json, key, cJSON_CreateString(val));
}

void Json::set(const char *key, const core::String &val) {
	set(key, val.c_str());
}

void Json::set(const char *key, int val) {
	if (_json == nullptr) {
		return;
	}
	addOrReplace(_json, key, cJSON_CreateNumber(val));
}

void Json::set(const char *key, double val) {
	if (_json == nullptr) {
		return;
	}
	addOrReplace(_json, key, cJSON_CreateNumber(val));
}

void Json::set(const char *key, float val) {
	set(key, (double)val);
}

void Json::set(const char *key, bool val) {
	if (_json == nullptr) {
		return;
	}
	addOrReplace(_json, key, cJSON_CreateBool(val));
}

void Json::push(const Json &val) {
	if (_json == nullptr) {
		return;
	}
	cJSON *dup = val._json ? cJSON_Duplicate(val._json, cJSON_True) : cJSON_CreateNull();
	cJSON_AddItemToArray(_json, dup);
}

void Json::push(const char *val) {
	if (_json == nullptr) {
		return;
	}
	cJSON_AddItemToArray(_json, cJSON_CreateString(val));
}

void Json::push(const core::String &val) {
	push(val.c_str());
}

void Json::push(int val) {
	if (_json == nullptr) {
		return;
	}
	cJSON_AddItemToArray(_json, cJSON_CreateNumber(val));
}

void Json::push(double val) {
	if (_json == nullptr) {
		return;
	}
	cJSON_AddItemToArray(_json, cJSON_CreateNumber(val));
}

void Json::push(float val) {
	push((double)val);
}

void Json::push(bool val) {
	if (_json == nullptr) {
		return;
	}
	cJSON_AddItemToArray(_json, cJSON_CreateBool(val));
}

int Json::size() const {
	if (_json == nullptr) {
		return 0;
	}
	return cJSON_GetArraySize(_json);
}

bool Json::empty() const {
	return size() == 0;
}

core::String Json::dump() const {
	if (_json == nullptr) {
		return "null";
	}
	char *printed = cJSON_PrintUnformatted(_json);
	if (printed == nullptr) {
		return "null";
	}
	core::String result(printed);
	cJSON_free(printed);
	return result;
}

core::String Json::dump(int /*indent*/) const {
	if (_json == nullptr) {
		return "null";
	}
	char *printed = cJSON_Print(_json);
	if (printed == nullptr) {
		return "null";
	}
	core::String result(printed);
	cJSON_free(printed);
	return result;
}

cJSON *Json::handle() const {
	return _json;
}

// Iterator implementation
Json::Iterator::Iterator(cJSON *item) : _item(item) {
}

Json::Iterator &Json::Iterator::operator++() {
	if (_item != nullptr) {
		_item = _item->next;
	}
	return *this;
}

bool Json::Iterator::operator!=(const Iterator &other) const {
	return _item != other._item;
}

Json Json::Iterator::operator*() const {
	return Json(_item, false);
}

core::String Json::Iterator::key() const {
	if (_item != nullptr && _item->string != nullptr) {
		return core::String(_item->string);
	}
	return "";
}

Json::Iterator Json::begin() const {
	if (_json == nullptr) {
		return Iterator(nullptr);
	}
	return Iterator(_json->child);
}

Json::Iterator Json::end() const {
	return Iterator(nullptr);
}

} // namespace json
