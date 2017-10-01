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

// TODO:
// * password support
// * relative updates
class DBHandler {
private:
	State execInternal(const std::string& query) const;
	State execInternalWithParameters(const std::string& query, Model& model, const BindParam& param) const;

public:
	DBHandler();

	bool init();
	void shutdown();

	template<class MODEL>
	bool deleteModel(MODEL&& model, const DBCondition& condition) const {
		int conditionAmount = 0;
		const std::string& query = createDeleteStatement(model) + createWhere(condition, conditionAmount);
		Log::debug("Execute query '%s'", query.c_str());
		ScopedConnection scoped(connection());
		if (!scoped) {
			Log::error("Could not execute query '%s' - could not acquire connection", query.c_str());
			return false;
		}
		State s(scoped.connection());
		if (conditionAmount > 0) {
			BindParam params(conditionAmount);
			for (int i = 0; i < conditionAmount; ++i) {
				const int index = params.add();
				const char* value = condition.value(i);
				Log::debug("Parameter %i: '%s'", index + 1, value);
				params.values[index] = value;
			}
			if (!s.exec(query.c_str(), conditionAmount, &params.values[0])) {
				Log::error("Failed to execute query '%s' with %i parameters", query.c_str(), conditionAmount);
				return false;
			}
		} else if (!s.exec(query.c_str())) {
			Log::error("Failed to execute query '%s'", query.c_str());
			return false;
		}
		if (s.affectedRows <= 0) {
			Log::trace("No rows affected, can't fill model values");
			return true;
		}
		Log::debug("Affected rows %i", s.affectedRows);
		return true;
	}

	template<class FUNC, class MODEL>
	bool select(MODEL&& model, const DBCondition& condition, FUNC&& func) const {
		int conditionAmount = 0;
		const std::string& select = createSelect(model) + createWhere(condition, conditionAmount);
		Log::debug("Execute query '%s'", select.c_str());
		ScopedConnection scoped(connection());
		if (!scoped) {
			Log::error("Could not execute query '%s' - could not acquire connection", select.c_str());
			return false;
		}
		State s(scoped.connection());
		if (conditionAmount > 0) {
			BindParam params(conditionAmount);
			for (int i = 0; i < conditionAmount; ++i) {
				const int index = params.add();
				const char* value = condition.value(i);
				Log::debug("Parameter %i: '%s'", index + 1, value);
				params.values[index] = value;
			}
			if (!s.exec(select.c_str(), conditionAmount, &params.values[0])) {
				Log::error("Failed to execute query '%s' with %i parameters", select.c_str(), conditionAmount);
				return false;
			}
		} else if (!s.exec(select.c_str())) {
			Log::error("Failed to execute query '%s'", select.c_str());
			return false;
		}
		if (s.affectedRows <= 0) {
			Log::trace("No rows affected, can't fill model values");
			return true;
		}
		for (int i = 0; i < s.affectedRows; ++i) {
			typename std::remove_reference<MODEL>::type selectedModel;
			selectedModel.fillModelValues(s);
			func(std::move(selectedModel));
			++s.currentRow;
		}
		Log::debug("Affected rows %i", s.affectedRows);
		return true;
	}

	template<class MODEL>
	bool select(MODEL& model, const DBCondition& condition) const {
		return select(model, condition, [&model] (MODEL&& selectedModel) {
			model = std::move(selectedModel);
		});
	}

	template<class MODEL>
	bool select(MODEL& model) const {
		return select(model, DBConditionOne());
	}

	Connection* connection() const;

	bool update(Model& model) const;
	bool insert(Model& model) const;
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
