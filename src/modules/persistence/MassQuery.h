/**
 * @file
 */

#pragma once

#include "BindParam.h"
#include <memory>
#include <vector>

namespace persistence {

class ISavable;
class DBHandler;

/**
 * @brief Implements mass updates for @c ISavable instances.
 */
class MassQuery {
private:
	const DBHandler * const _dbHandler;
	const size_t _commitSize;
	std::vector<const Model*> _insertOrUpdate;
	std::vector<const Model*> _delete;
	friend class DBHandler;
	MassQuery(const DBHandler* dbHandler, size_t amount = 1000);

public:
	~MassQuery();

	void add(ISavable* savable);
	void commit();
};

}
