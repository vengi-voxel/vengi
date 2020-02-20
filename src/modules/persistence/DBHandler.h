/**
 * @file
 *
 * @defgroup Persistence
 * @{
 * The @c DBHandler is used to interact with the database. The @c DatabaseTool is used to generate the metadata classes
 * for the database tables.
 */

#pragma once

#include "PersistenceModels.h"
#include "ConnectionPool.h"
#include "Model.h"
#include "MassQuery.h"
#include "core/StringUtil.h"
#include "core/Log.h"
#include "core/IComponent.h"
#include "ScopedConnection.h"
#include "BindParam.h"
#include "SQLGenerator.h"
#include "DBCondition.h"
#include "OrderBy.h"
#include <memory>
#include <vector>

namespace persistence {

/**
 * @brief Database access for insert, update, delete, ...
 * @ingroup Persistence
 * @sa DatabaseTool
 * @sa Model
 * @todo password support
 */
class DBHandler : public core::IComponent {
private:
	friend class MassQuery;
	static constexpr auto logid = Log::logid("DBHandler");
	State execInternal(const core::String& query) const;
	State execInternalWithParameters(const core::String& query, Model& model, const BindParam& param) const;
	State execInternalWithCondition(const core::String& query, BindParam& params, int conditionOffset, const DBCondition& condition) const;
	State execInternalWithParameters(const core::String& query, const BindParam& param) const;

	mutable ConnectionPool _connectionPool;

	virtual Connection* connection() const;

	bool insertMetadata(const Model& model) const;
	bool loadMetadata(const Model& model, std::vector<db::MetainfoModel>& schemaModels) const;

	template<class FUNC, class MODEL>
	bool select(const core::String& query, const BindParam& keyParams, int conditionAmount, MODEL& model, const DBCondition& condition, FUNC&& func) const {
		Log::debug(logid, "Execute query '%s'", query.c_str());
		ScopedConnection scoped(_connectionPool, connection());
		if (!scoped) {
			Log::error(logid, "Could not execute query '%s' - could not acquire connection", query.c_str());
			return false;
		}
		State s(scoped.connection());
		if (conditionAmount > 0) {
			if (keyParams.position == conditionAmount) {
				if (!s.exec(query.c_str(), conditionAmount, &keyParams.values[0], &keyParams.lengths[0], &keyParams.formats[0])) {
					Log::error(logid, "Failed to execute query '%s' with %i parameters", query.c_str(), conditionAmount);
				}
			} else {
				BindParam params = keyParams;
				for (int i = keyParams.position; i < conditionAmount; ++i) {
					const int index = params.add();
					const char* value = condition.value(i);
					Log::debug(logid, "Parameter %i: '%s'", index + 1, value);
					params.values[index] = value;
				}
				if (!s.exec(query.c_str(), conditionAmount, &params.values[0], &params.lengths[0], &params.formats[0])) {
					Log::error(logid, "Failed to execute query '%s' with %i parameters", query.c_str(), conditionAmount);
				}
			}
		} else if (!s.exec(query.c_str())) {
			Log::error(logid, "Failed to execute query '%s'", query.c_str());
		}
		for (int i = 0; i < s.affectedRows; ++i) {
			typename core::remove_reference<MODEL>::type selectedModel;
			selectedModel.fillModelValues(s);
			func(core::move(selectedModel));
		}
		return s.result;
	}

	bool _initialized = false;
	const bool _useForeignKeys;

public:
	DBHandler(bool useForeignKeys = true);
	~DBHandler();

	/**
	 * @brief Initializes the connections
	 * @return @c true if the initialization was executed successfully, @c false otherwise.
	 */
	bool init() override;

	/**
	 * @brief Not calling @c shutdown() after @c init() was called will lead to memory leaks
	 */
	void shutdown() override;

	/**
	 * @brief Deletes one or more database entries of the given @c persistence::Model
	 * @param[in] model The model that should be deleted
	 * @param[in] condition The @c persistence::DBCondition that identifies the entries to delete
	 * @return @c true if the statement was executed successfully, @c false otherwise.
	 */
	template<class MODEL>
	bool deleteModel(MODEL&& model, const DBCondition& condition = DBConditionOne()) const {
		BindParam params(10);
		const core::String& stmt = createDeleteStatement(model, &params);
		int conditionAmount = params.position;
		const core::String& where = createWhere(condition, conditionAmount);
		const core::String& query = stmt + where;
		const int conditionOffset = conditionAmount - params.position;
		if (conditionOffset > 0) {
			return execInternalWithCondition(query, params, conditionOffset, condition).result;
		}
		return execInternalWithParameters(query, params).result;
	}

	/**
	 * @brief Select database entries of the given @c persistence::Model
	 * @param[in] model The model that should be selected
	 * @param[in] condition The @c persistence::DBCondition that identifies the entries to select
	 * @param[in] func The callback that is notified on every entry that was found that matches
	 * the search conditions. It accepts a rvalue of the given @c MODEL class as parameter.
	 * @return @c true if the statement was executed successfully, @c false otherwise.
	 */
	template<class FUNC, class MODEL>
	bool select(MODEL&& model, const DBCondition& condition, FUNC&& func) const {
		BindParam params(10);
		const core::String& stmt = createSelect(model, &params);
		int conditionAmount = params.position;
		const core::String& where = createWhere(condition, conditionAmount);
		const core::String& query = stmt + where;
		return select(query, params, conditionAmount, model, condition, func);
	}

	/**
	 * @brief Select database entries of the given @c persistence::Model
	 * @param[in] model The model that should be selected
	 * @param[in] condition The @c persistence::DBCondition that identifies the entries to select
	 * @param[in] orderBy The field data to order by
	 * @param[in] func The callback that is notified on every entry that was found that matches
	 * the search conditions. It accepts a rvalue of the given @c MODEL class as parameter.
	 * @return @c true if the statement was executed successfully, @c false otherwise.
	 */
	template<class FUNC, class MODEL>
	bool select(MODEL&& model, const DBCondition& condition, const OrderBy& orderBy, FUNC&& func) const {
		BindParam params(10);
		const core::String& stmt = createSelect(model, &params);
		int conditionAmount = params.position;
		const core::String& where = createWhere(condition, conditionAmount);
		const core::String& query = stmt + where + createOrderBy(orderBy) + createLimitOffset(orderBy.range);
		return select(query, params, conditionAmount, model, condition, func);
	}

	/**
	 * @brief Select database entries of the given @c persistence::Model
	 * @param[in] model The model that should be selected
	 * @param[in] orderBy The field data to order by
	 * @param[in] func The callback that is notified on every entry that was found that matches
	 * the search conditions. It accepts a rvalue of the given @c MODEL class as parameter.
	 * @return @c true if the statement was executed successfully, @c false otherwise.
	 */
	template<class FUNC, class MODEL>
	bool select(MODEL&& model, const OrderBy& orderBy, FUNC&& func) const {
		return select(model, DBConditionOne(), orderBy, func);
	}

	/**
	 * @brief Select one database entry of the given @c persistence::Model (or if the result leads to multiple
	 * entries, you get the last one - but keep in mind that the result set is not ordered!)
	 * @param[in,out] model The model that should be selected - the result data is written into the model
	 * @param[in] condition The @c persistence::DBCondition that identifies the entries to select
	 * @return @c true if the statement was executed successfully, @c false otherwise.
	 */
	template<class MODEL>
	bool select(MODEL& model, const DBCondition& condition) const {
		return select(model, condition, [&model] (MODEL&& selectedModel) {
			model = core::move(selectedModel);
		});
	}

	void freeBlob(Blob& blob) const;

	/**
	 * @brief Updates the database entry for the give model. The primary keys must be set in the
	 * @c persistence::Model instance that is given to this method
	 * @param[in,out] model The model that should be updated
	 * @return @c true if the statement was executed successfully, @c false otherwise.
	 */
	bool update(Model& model, const DBCondition& condition = DBConditionOne()) const;

	/**
	 * @brief Insert or updates the database entry for the give model. The primary keys must be set in the
	 * @c persistence::Model instance that is given to this method. If you violate a unique key constraint,
	 * an update is performed instead. Depending on the @c persistence::Field settings you either get a
	 * relative update or an absolute set during that conflict for the new data.
	 * @param[in,out] model The model that should be inserted/updated
	 * @return @c true if the statement was executed successfully, @c false otherwise.
	 */
	bool insert(Model& model) const;

	bool insert(Model&& model) const;

	bool insert(std::vector<const Model*>& models) const;

	template<class MODEL>
	bool insert(std::vector<MODEL>& models) const {
		std::vector<const Model*> converted(models.size());
		const size_t size = models.size();
		for (size_t i = 0u; i < size; ++i) {
			converted[i] = &models[i];
		}
		return insert(converted);
	}

	template<class MODEL>
	bool deleteModels(std::vector<MODEL>& models) const {
		// TODO: prepared statement
		for (auto& m : models) {
			if (!deleteModel(m)) {
				return false;
			}
		}
		return true;
	}

	bool deleteModels(std::vector<const Model*>& models) const;

	/**
	 * @brief Truncate the table for the given @c persistence::Model
	 * @param[in] model The model that identifies the table that should be truncated
	 * @return @c true if the statement was executed successfully, @c false otherwise.
	 */
	bool truncate(const Model& model) const;

	/**
	 * @brief Truncate the table for the given @c persistence::Model
	 * @param[in] model The model that identifies the table that should be truncated
	 * @return @c true if the statement was executed successfully, @c false otherwise.
	 */
	bool truncate(Model&& model) const;

	MassQuery massQuery() const;

	bool dropTable(const Model& model) const;
	bool dropTable(Model&& model) const;
	bool tableExists(const Model& model) const;

	/**
	 * @brief Create the table for the given @c persistence::Model
	 * @param[in] model The model that identifies the table that should be created
	 * @return @c true if the statement was executed successfully, @c false otherwise.
	 */
	virtual bool createTable(Model&& model) const;

	/**
	 * @brief Create a table for the given @c persistence::Model - or if this table already exists, it checks
	 * whether the given model meta data matches the existing table and perform update statements if the differ
	 * @return @c true if the statement was executed successfully, @c false otherwise.
	 */
	virtual bool createOrUpdateTable(Model&& model) const;

	/**
	 * @brief Executes a single query
	 * @param[in] query The query to execute
	 * @return @c true if the statement was executed successfully, @c false otherwise.
	 */
	virtual bool exec(const core::String& query) const;

	// transactions
	bool begin();
	bool commit();
	bool rollback();
};

typedef std::shared_ptr<DBHandler> DBHandlerPtr;

}

/**
 * @}
 */
