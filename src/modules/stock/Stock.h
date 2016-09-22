/**
 * @file
 */

#pragma once

#include "Item.h"
#include "Inventory.h"
#include <vector>

namespace stock {

/**
 * @defgroup Stock
 * @{
 * @brief The Stock class manages Items. All the items that someone owns are stored in this class.
 *
 * The stock handler is taking responsibility for putting the items into it's Inventory. The Inventory itself
 * only has access to pointers over all of the items.
 */
class Stock {
private:
	/** All the items this instance can deal with */
	std::vector<Item*> _items;
	/** The inventory has pointers to all the items distributed over all the Container instances in the Inventory. */
	Inventory _inventory;

	inline auto find(const ItemId& itemId) const {
		return std::find_if(_items.begin(), _items.end(), [itemId] (const Item* item) {
			return item->id() == itemId;
		});
	}

	inline auto find(const ItemId& itemId) {
		return std::find_if(_items.begin(), _items.end(), [itemId] (const Item* item) {
			return item->id() == itemId;
		});
	}
public:
	Stock();

	/**
	 * @brief Initializes the Stock class with all the Item instances it should manage.
	 */
	void init(const std::vector<Item*>& items);

	int add(Item* item);

	int remove(Item* item);

	int count(const ItemType& itemType) const;

	int count(ItemId itemId) const;

	const Inventory& inventory() const;
	Inventory& inventory();
};

inline const Inventory& Stock::inventory() const {
	return _inventory;
}

inline Inventory& Stock::inventory() {
	return _inventory;
}

/**
 * @}
 */

}
