/**
 * @file
 */

#pragma once

#include "backend/ForwardDecl.h"
#include "stock/Stock.h"
#include <memory>

namespace backend {

class User;

/**
 * @brief User stock manager that restores the state on login
 */
class UserStockMgr {
private:
	User* _user;
	const stock::StockProviderPtr _stockDataProvider;
	persistence::DBHandlerPtr _dbHandler;
	stock::Stock _stock;

public:
	UserStockMgr(User* user, const stock::StockProviderPtr& stockDataProvider, const persistence::DBHandlerPtr& dbHandler);

	bool init();
	void shutdown();

	void update(long dt);
};

typedef std::shared_ptr<UserStockMgr> StockMgrPtr;

}
