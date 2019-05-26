/**
 * @file
 */

#pragma once

#include "ForwardDecl.h"
#include "core/NonCopyable.h"
#include <string>

namespace persistence {

/**
 * @brief Wraps the postgres api
 */
class State : public core::NonCopyable {
private:
	Connection* _connection = nullptr;
	void checkLastResult(ConnectionType* connection);
public:
	constexpr State() {
	}

	State(Connection* connection);
	State(State&& other);
	~State();

	bool exec(const char* statement, int parameterCount = 0, const char *const *paramValues = nullptr);
	bool prepare(const char *name, const char* statement, int parameterCount);
	bool execPrepared(const char *name, int parameterCount, const char *const *paramValues);

	/**
	 * @param[in] colIndex The column index of the current row. Starting at index 0 for the first column
	 */
	const char *columnName(int colIndex) const;

	/**
	 * @param[in] colIndex The column index of the current row. Starting at index 0 for the first column
	 * @param[out] value The value of the current row and given colIndex as string
	 * @param[out] length The length of the value string
	 * @param[out] isNull @c true if the result was null
	 */
	void getResult(int colIndex, const char **value, int *length, bool *isNull) const;

	/**
	 * @param[in] colIndex The column index of the current row. Starting at index 0 for the first column
	 */
	int asInt(int colIndex) const;

	/**
	 * @param[in] colIndex The column index of the current row. Starting at index 0 for the first column
	 */
	bool asBool(int colIndex) const;
	static bool isBool(const char *value);

	ResultType* res = nullptr;

	char* lastErrorMsg = nullptr;
	int affectedRows = -1;
	int cols = -1;
	int currentRow = -1;
	// false on error, true on success
	bool result = false;
};

}
