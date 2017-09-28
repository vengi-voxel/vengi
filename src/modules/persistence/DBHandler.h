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
#include "SQLGenerator.h"
#include <memory>

namespace persistence {

class DBHandler {
private:
	State execInternal(const std::string& query) const;

public:
	DBHandler();

	bool init();

	void shutdown();

	template<class FUNC, class MODEL>
	bool selectAll(MODEL&& model, FUNC&& func) {
		const std::string& select = createSelect(model);
		ScopedConnection scoped(core::Singleton<ConnectionPool>::getInstance().connection());
		if (!scoped) {
			Log::error("Could not execute query '%s' - could not acquire connection", select.c_str());
			return false;
		}
		State s(scoped.connection());
		if (!s.exec(select.c_str())) {
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

	bool truncate(const Model& model) const;

	bool truncate(Model&& model) const;

	bool createTable(Model&& model) const;

	bool exec(const std::string& query) const;

	// transactions
	bool begin();
	bool commit();
	bool rollback();
};

typedef std::shared_ptr<DBHandler> DBHandlerPtr;

}
