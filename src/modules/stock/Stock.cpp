/**
 * @file
 */

#include "Stock.h"
#include <algorithm>

namespace stock {

Stock::Stock() {
}

void Stock::init(const std::vector<Item*>& items) {
	Log::debug("Initialize stock with %i items", (int) items.size());
	for (Item* i : items) {
		add(i);
	}
}

int Stock::add(Item* item) {
	// TODO: max stock count check
	Log::debug("Add item %s", item->data().name());
	_items.push_back(item);
	return 1;
}

int Stock::remove(Item* item) {
	auto i = find(item->id());
	if (i == _items.end()) {
		return 0;
	}
	_items.erase(i);
	_inventory.notifyRemove(item);
	return 1;
}

int Stock::count(const ItemType& itemType) const {
	return std::count(_items.begin(), _items.end(), itemType);
}

int Stock::count(ItemId itemId) const {
	return std::count_if(_items.begin(), _items.end(), [=] (const Item* item) {return item->id() == itemId;});
}

}
