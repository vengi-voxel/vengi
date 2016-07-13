/**
 * @file
 */

#pragma once

#include "Connection.h"
#include "PeristenceModel.h"
#include <unordered_map>
#include <libpq-fe.h>
#include "core/NonCopyable.h"

namespace persistence {

typedef std::unordered_map<std::string, std::string> KeyValueMap;
typedef std::pair<std::string, std::string> KeyValuePair;

class Store : public core::NonCopyable {
private:
	Connection* _connection;

	class State {
	public:
		State(PGresult* res);
		~State();

		PGresult* res = nullptr;
		std::string lastErrorMsg;
		ExecStatusType lastState = PGRES_FATAL_ERROR;
		int affectedRows = -1;
		bool result = false;
	};

	std::string sqlBuilder(const PeristenceModel& model, bool update) const;

	std::string sqlLoadBuilder(const PeristenceModel& model) const;
	State query(const std::string& query) const;
	bool checkLastResult(State& state) const;
	PGresult* result() const;

public:
	Store(Connection* conn);
	~Store();

	bool begin() const;

	bool end() const;

	bool store(const PeristenceModel& model) const;

	bool createTable(const PeristenceModel& model) const;

	KeyValueMap load(const PeristenceModel& model) const;
};

}
