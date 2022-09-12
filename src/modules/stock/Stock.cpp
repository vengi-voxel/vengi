/**
 * @file
 */

#include "Stock.h"
#include "Shape.h"
#include "stock/StockDataProvider.h"
#include "core/Log.h"

namespace stock {

Stock::Stock(const StockDataProviderPtr& stockDataProvider) :
		_inventory(), _stockDataProvider(stockDataProvider) {
}

bool Stock::init() {
	for (const auto& entry : _stockDataProvider->containers()) {
		const ContainerData* data = entry.second;
		if (!_inventory.initContainer(data->id, data->shape, data->flags)) {
			Log::error("Failed to init inventory container with name '%s'", entry.first.c_str());
			return false;
		}
		Log::debug("Initialized container %i with name %s", (int)data->id, entry.first.c_str());
	}
	return true;
}

void Stock::shutdown() {
	_inventory.clear();
	_items.clear();
}

int Stock::containerId(const core::String& name) const {
	const ContainerData* data = _stockDataProvider->containerData(name);
	if (data == nullptr) {
		Log::warn("Could not resolve container for '%s'", name.c_str());
		return -1;
	}
	return data->id;
}

ItemPtr Stock::add(const ItemPtr& item) {
	Log::debug("Add item %s", item->data().name());
	if (item->amount() == 0) {
		Log::debug("Given amount was 0 - ignore item add");
		return ItemPtr();
	}
	auto i = find(item->id());
	if (i == _items.end()) {
		_items.put(item->id(), item);
		return item;
	}
	ItemPtr alreadyExisting = i->value;
	alreadyExisting->changeAmount(item->amount());
	return alreadyExisting;
}

ItemAmount Stock::remove(const ItemPtr& item) {
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

ItemAmount Stock::count(const ItemType& itemType) const {
	ItemAmount n = 0;
	for (const auto *entry : _items) {
		const ItemPtr& i = entry->value;
		if (i->type() == itemType) {
			n += i->amount();
		}
	}
	return n;
}

ItemAmount Stock::count(ItemId itemId) const {
	auto i = find(itemId);
	if (i == _items.end()) {
		return 0;
	}
	return i->second->amount();
}

}
