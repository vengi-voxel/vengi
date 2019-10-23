/**
 * @file
 */

#include "Stock.h"
#include "Shape.h"
#include "stock/StockDataProvider.h"
#include "core/Log.h"
#include <algorithm>

namespace stock {

Stock::Stock(const StockDataProviderPtr& stockDataProvider) :
		_inventory(), _stockDataProvider(stockDataProvider) {
}

bool Stock::init() {
	for (const auto& entry : _stockDataProvider->containers()) {
		const ContainerData* data = entry.second;
		if (!_inventory.initContainer(data->id, data->shape, data->flags)) {
			return false;
		}
	}
	return true;
}

void Stock::shutdown() {
	_inventory.clear();
	_items.clear();
}

ItemPtr Stock::add(const ItemPtr& item) {
	Log::debug("Add item %s", item->data().name());
	if (item->amount() == 0) {
		Log::debug("Given amount was 0 - ignore item add");
		return ItemPtr();
	}
	auto i = find(item->id());
	if (i == _items.end()) {
		_items.insert(std::make_pair(item->id(), item));
		return item;
	}
	ItemPtr alreadyExisting = i->second;
	alreadyExisting->changeAmount(item->amount());
	return alreadyExisting;
}

int Stock::remove(const ItemPtr& item) {
	auto i = find(item->id());
	if (i == _items.end()) {
		return 0;
	}
	const ItemAmount amount = i->second->changeAmount(-item->amount());
	if (amount <= 0) {
		_items.erase(i);
		_inventory.notifyRemove(item);
		return 0;
	}
	return amount;
}

int Stock::count(const ItemType& itemType) const {
	int n = 0;
	for (const auto& entry : _items) {
		const ItemPtr& i = entry.second;
		if (i->type() == itemType) {
			n += i->amount();
		}
	}
	return n;
}

int Stock::count(ItemId itemId) const {
	auto i = find(itemId);
	if (i == _items.end()) {
		return 0;
	}
	return i->second->amount();
}

}
