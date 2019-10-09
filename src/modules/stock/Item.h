/**
 * @file
 */

#pragma once

#include "ItemData.h"
#include "Container.h"
#include <memory>

namespace stock {

using ItemAmount = int64_t;

/**
 * @ingroup Stock
 */
class Item {
protected:
	const ItemData& _data;
	ItemAmount _amount = 0;
public:
	Item(const ItemData& data);

	ItemId id() const;

	const char* name() const;

	const ItemType& type() const;

	const ItemShape& shape() const;

	const ItemData& data() const;

	ItemAmount changeAmount(ItemAmount delta);

	ItemAmount amount() const;

	bool operator==(ItemType type) const;

	bool operator==(ItemId id) const;
};

inline ItemAmount Item::amount() const {
	return _amount;
}

inline ItemAmount Item::changeAmount(ItemAmount delta) {
	_amount += delta;
	return _amount;
}

inline bool Item::operator==(ItemType type) const {
	return this->type() == type;
}

inline bool Item::operator==(ItemId id) const {
	return this->id() == id;
}

inline const ItemType& Item::type() const {
	return data().type();
}

inline const ItemShape& Item::shape() const {
	return data().shape();
}

inline ItemId Item::id() const {
	return data().id();
}

inline const char* Item::name() const {
	return data().name();
}

inline const ItemData& Item::data() const {
	return _data;
}

inline bool operator==(const Item* item, ItemType type) {
	return item->type() == type;
}

typedef std::shared_ptr<Item> ItemPtr;

}
