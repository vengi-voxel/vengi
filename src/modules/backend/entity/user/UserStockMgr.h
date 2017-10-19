/**
 * @file
 */

#pragma once

#include "stock/Stock.h"
#include "stock/ItemProvider.h"
#include "persistence/DBHandler.h"
#include <memory>

namespace backend {

class User;

/**
 * @brief User stock manager that restores the state on login
 */
class UserStockMgr {
private:
	User* _user;
	const stock::ItemProviderPtr _itemProvider;
	persistence::DBHandlerPtr _dbHandler;
	stock::Stock _stock;

public:
	UserStockMgr(User* user, const stock::ItemProviderPtr& itemProvider, const persistence::DBHandlerPtr& dbHandler);

	void init();
	void shutdown();

	void update(long dt);
};

typedef std::shared_ptr<UserStockMgr> StockMgrPtr;

}
