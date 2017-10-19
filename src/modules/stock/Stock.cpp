/**
 * @file
 */

#include "Stock.h"
#include "core/Log.h"
#include <algorithm>

namespace stock {

Stock::Stock() {
}

void Stock::init(const std::vector<ItemPtr>& items) {
	Log::debug("Initialize stock with %i items", (int) items.size());
	for (const ItemPtr& i : items) {
		add(i);
	}
}

ItemPtr Stock::add(const ItemPtr& item) {
	Log::debug("Add item %s", item->data().name());
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
	_items.erase(i);
	_inventory.notifyRemove(item);
	return item->amount();
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
