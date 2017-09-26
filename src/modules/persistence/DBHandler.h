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

namespace persistence {

class DBHandler {
private:
	static std::string quote(const std::string& in) {
		return core::string::format("\"%s\"", in.c_str());
	}
public:
	DBHandler();

	bool init();

	void shutdown();

	static std::string createSelect(const Model& model);
	static std::string createSelect(const Model::Fields& fields, const std::string& tableName);

	template<class FUNC, class MODEL>
	bool selectAll(MODEL&& model, FUNC&& func) {
		const std::string& select = createSelect(model);
		ScopedConnection scoped(core::Singleton<ConnectionPool>::getInstance().connection());
		if (!scoped) {
			Log::error("Could not execute query '%s' - could not acquire connection", select.c_str());
			return false;
		}
		ConnectionType* conn = scoped.connection()->connection();
		Model::State s(PQexec(conn, select.c_str()));
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

	template<class MODEL>
	static void truncate(const MODEL& model) {
		model.exec("TRUNCATE TABLE " + quote(model.tableName()));
	}

	template<class MODEL>
	static void truncate(MODEL&& model) {
		model.exec("TRUNCATE TABLE " + quote(model.tableName()));
	}
};

typedef std::shared_ptr<DBHandler> DBHandlerPtr;

}
