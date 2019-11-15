/**
 * @file
 */

#include "UserStockMgr.h"
#include "BackendModels.h"
#include "persistence/DBHandler.h"
#include "stock/StockDataProvider.h"
#include "backend/entity/User.h"
#include "core/Log.h"

namespace backend {

UserStockMgr::UserStockMgr(User* user, const stock::StockDataProviderPtr& stockDataProvider, const persistence::DBHandlerPtr& dbHandler) :
		_user(user), _dbHandler(dbHandler), _stock(stockDataProvider) {
}

void UserStockMgr::update(long dt) {
}

bool UserStockMgr::init() {
	_stock.init();
	const EntityId userId = _user->id();
	if (!_dbHandler->select(db::InventoryModel(), db::DBConditionInventoryModelUserid(userId), [this] (db::InventoryModel&& model) {
		stock::Inventory& inventory = _stock.inventory();
		const stock::ItemPtr& item = _stockDataProvider->createItem(model.itemid());
		if (!item) {
			Log::warn("Could not get item for %i", model.itemid());
			return;
		}
		inventory.add(model.containerid(), item, model.x(), model.y());
	})) {
		Log::warn("Could not load inventory for user " PRIEntId, userId);
	}
	return true;
}

void UserStockMgr::shutdown() {
	const EntityId userId = _user->id();
	Log::info("Shutdown stock manager for user " PRIEntId, userId);
	const stock::Inventory& inventory = _stock.inventory();
	const int maxContainers = inventory.maxContainers();
	for (int i = 0; i < maxContainers; ++i) {
		const stock::Container* container = inventory.container(i);
		if (container == nullptr) {
			continue;
		}
		const stock::Container::ContainerItems& items = container->items();
		for (const stock::Container::ContainerItem& item : items) {
			db::InventoryModel model;
			model.setContainerid(i);
			model.setUserid(userId);
			model.setItemid(item.item->id());
			model.setX(item.x);
			model.setY(item.y);
			_dbHandler->insert(model);
		}
	}
	_stock.shutdown();
}

}
