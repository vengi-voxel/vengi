/**
 * @file
 * @brief Lightweight JSON wrapper around cJSON for fast compilation.
 *
 * Only forward-declares cJSON in the header to minimize include cost.
 * All cJSON calls are in the .cpp file.
 */

#pragma once

#include "core/String.h"

struct cJSON;

namespace json {

/**
 * @brief Lightweight wrapper around cJSON.
 *
 * Supports owned and non-owned (borrowed) semantics. Owned instances
 * free the underlying cJSON tree on destruction. Non-owned instances
 * are lightweight views into a parent tree.
 */
class Json {
public:
	Json();
	~Json();
	Json(const Json &other);
	Json(Json &&other) noexcept;
	Json &operator=(const Json &other);
	Json &operator=(Json &&other) noexcept;

	/**
	 * @brief Wrap an existing cJSON pointer.
	 * @param json The cJSON pointer to wrap. May be nullptr.
	 * @param owned If true, Json takes ownership and will free the cJSON tree on destruction.
	 */
	explicit Json(cJSON *json, bool owned = true);

	/** @brief Parse a JSON string. Returns an owned Json. Check isValid() for success. */
	static Json parse(const char *str);
	/** @brief Parse a JSON string. Returns an owned Json. Check isValid() for success. */
	static Json parse(const core::String &str);
	/** @brief Validate whether the given string is valid JSON without parsing. */
	static bool accept(const char *str);
	/** @brief Create an empty JSON object. */
	static Json object();
	/** @brief Create an empty JSON array. */
	static Json array();

	bool isValid() const;

	// Type checks
	bool isNull() const;
	bool isObject() const;
	bool isArray() const;
	bool isString() const;
	bool isBool() const;
	bool isNumber() const;
	bool isNumberInteger() const;
	bool isNumberFloat() const;

	/**
	 * @brief Check whether a key exists in this object.
	 */
	bool contains(const char *key) const;

	/**
	 * @brief Get a child element by key. Returns a non-owned view.
	 * If the key does not exist, returns an invalid Json (isValid() == false).
	 */
	Json get(const char *key) const;

	/**
	 * @brief Get an array element by index. Returns a non-owned view.
	 */
	Json get(int index) const;

	// Value extraction - returns the value of this node directly
	core::String str() const;
	const char *cStr() const;
	int intVal() const;
	double doubleVal() const;
	float floatVal() const;
	bool boolVal() const;

	// Safe value-with-default: extract child by key with fallback
	core::String strVal(const char *key, const char *defaultVal) const;
	int intVal(const char *key, int defaultVal) const;
	double doubleVal(const char *key, double defaultVal) const;
	float floatVal(const char *key, float defaultVal) const;
	bool boolVal(const char *key, bool defaultVal) const;

	// Mutation - set a child on this object
	void set(const char *key, const Json &val);
	void set(const char *key, const char *val);
	void set(const char *key, const core::String &val);
	void set(const char *key, int val);
	void set(const char *key, double val);
	void set(const char *key, float val);
	void set(const char *key, bool val);

	// Array operations
	void push(const Json &val);
	void push(const char *val);
	void push(const core::String &val);
	void push(int val);
	void push(double val);
	void push(float val);
	void push(bool val);

	// Aliases for push - for compatibility
	inline void push_back(const Json &val) { push(val); }
	inline void push_back(const char *val) { push(val); }
	inline void push_back(const core::String &val) { push(val); }
	inline void push_back(int val) { push(val); }
	inline void push_back(double val) { push(val); }
	inline void push_back(float val) { push(val); }
	inline void push_back(bool val) { push(val); }

	int size() const;
	bool empty() const;

	/** @brief Serialize to a JSON string. */
	core::String dump() const;
	/** @brief Serialize to a formatted JSON string with the given indentation level. */
	core::String dump(int indent) const;

	/** @brief Access the underlying cJSON pointer. */
	cJSON *handle() const;

	/**
	 * @brief Iterator for traversing children of objects or arrays.
	 */
	class Iterator {
	public:
		Iterator(cJSON *item);
		Iterator &operator++();
		bool operator!=(const Iterator &other) const;
		Json operator*() const;
		core::String key() const;

	private:
		cJSON *_item;
	};

	Iterator begin() const;
	Iterator end() const;

private:
	cJSON *_json;
	bool _owned;
};

/** @brief Extract a string value by key with an optional default. */
inline core::String toStr(const Json &json, const char *key, const core::String &defaultValue = "") {
	return json.strVal(key, defaultValue.c_str());
}

/** @brief Convert a Json string node to core::String. */
inline core::String toStr(const Json &json) {
	return json.str();
}

} // namespace json
