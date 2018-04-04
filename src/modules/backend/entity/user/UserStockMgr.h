/**
 * @file
 */

#pragma once

#include "backend/ForwardDecl.h"
#include "stock/Stock.h"
#include "core/IComponent.h"
#include <memory>

namespace backend {

class User;

/**
 * @brief User stock manager that restores the state on login
 */
class UserStockMgr : public core::IComponent {
private:
	User* _user;
	const stock::StockDataProviderPtr _stockDataProvider;
	persistence::DBHandlerPtr _dbHandler;
	stock::Stock _stock;

public:
	UserStockMgr(User* user, const stock::StockDataProviderPtr& stockDataProvider, const persistence::DBHandlerPtr& dbHandler);

	bool init() override;
	void shutdown() override;

	void update(long dt);
};

typedef std::shared_ptr<UserStockMgr> StockMgrPtr;

}
