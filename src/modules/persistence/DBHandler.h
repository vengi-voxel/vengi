/**
 * @file
 */

#pragma once

#include "Model.h"
#include "core/String.h"
#include "core/Log.h"
#include "ScopedConnection.h"
#include "PreparedStatement.h"
#include "SQLGenerator.h"
#include "DBCondition.h"
#include <memory>

namespace persistence {

class DBHandler {
private:
	State execInternal(const std::string& query) const;
	State execInternalWithParameters(const std::string& query, const Model& model) const;

public:
	DBHandler();

	bool init();
	void shutdown();

	template<class FUNC, class MODEL>
	bool select(MODEL&& model, FUNC&& func, const DBCondition& condition) {
		int conditionAmount;
	    const std::string& select = createSelect(model) + createWhere(model, condition, conditionAmount);
		ScopedConnection scoped(connection());
		if (!scoped) {
			Log::error("Could not execute query '%s' - could not acquire connection", select.c_str());
			return false;
		}
		State s(scoped.connection());
		if (!s.prepare("", select.c_str(), conditionAmount)) {
			return false;
		}
		State prepState(scoped.connection());
		BindParam params(conditionAmount);
	    for (int i = 0; i < conditionAmount; ++i) {
	    	const int index = params.add();
	    	const std::string& value = condition.value(i, model);
	    	params.valueBuffers.emplace_back(value);
	    	params.values[index] = params.valueBuffers.back().data();
	    }
		prepState.execPrepared("", conditionAmount, &params.values[0]);
		if (!prepState.result) {
			return false;
		}
		if (prepState.affectedRows <= 0) {
			Log::trace("No rows affected, can't fill model values");
			return true;
		}
		for (int i = 0; i < s.affectedRows; ++i) {
			std::shared_ptr<MODEL> modelptr = std::make_shared<MODEL>();
			modelptr->fillModelValues(s);
			func(modelptr);
			++s.currentRow;
		}
		return true;
	}

	template<class FUNC, class MODEL>
	bool select(MODEL&& model, FUNC&& func) {
	    return select(std::forward<MODEL>(model), std::forward<FUNC>(func), DBConditionOne());
	}

	Connection* connection() const;

	bool insert(const Model& model) const;
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
