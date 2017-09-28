/**
 * @file
 */

#pragma once

#include "Model.h"
#include "core/String.h"
#include "core/Log.h"
#include "core/Singleton.h"
#include "ScopedConnection.h"
#include "ConnectionPool.h"
#include <memory>

// TODO: remove me!
#include <libpq-fe.h>

namespace persistence {

class DBHandler {
private:
	static std::string quote(const std::string& in);

	State execInternal(const std::string& query) const;

	bool checkLastResult(State& state, Connection* connection) const;
public:
	static std::string getDbType(const Field& field);
	static std::string getDbFlags(int numberPrimaryKeys, const Constraints& constraints, const Field& field);

	DBHandler();

	bool init();

	void shutdown();

	static std::string createCreateTableStatement(const Model& table);
	static std::string createSelect(const Model& model);
	static std::string createSelect(const Fields& fields, const std::string& tableName);

	template<class FUNC, class MODEL>
	bool selectAll(MODEL&& model, FUNC&& func) {
		const std::string& select = createSelect(model);
		ScopedConnection scoped(core::Singleton<ConnectionPool>::getInstance().connection());
		if (!scoped) {
			Log::error("Could not execute query '%s' - could not acquire connection", select.c_str());
			return false;
		}
		ConnectionType* conn = scoped.connection()->connection();
		State s(PQexec(conn, select.c_str()));
		if (!model.checkLastResult(s, scoped)) {
			return false;
		}
		for (int i = 0; i < s.affectedRows; ++i) {
			std::shared_ptr<MODEL> modelptr = std::make_shared<MODEL>();
			modelptr->fillModelValues(s);
			func(modelptr);
			++s.currentRow;
		}
		return true;
	}

	void truncate(const Model& model) const;

	void truncate(Model&& model) const;

	bool createTable(Model&& model) const;

	bool exec(const std::string& query) const;
};

typedef std::shared_ptr<DBHandler> DBHandlerPtr;

}
