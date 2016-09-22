/**
 * @file
 */

#pragma once

#include "ItemData.h"
#include "Container.h"

namespace stock {

using ItemAmount = int64_t;

/**
 * @ingroup Stock
 */
class Item {
protected:
	static constexpr uint32_t ItemInContainer = 1 << 0;
	const ItemData& _data;
	Container _container; // socket for other items
	uint32_t _flags = 0u;
	ItemAmount _amount = 0;
public:
	Item(const ItemData& data);

	ItemId id() const;

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

inline const ItemData& Item::data() const {
	return _data;
}

inline bool operator==(const Item* item, ItemType type) {
	return item->type() == type;
}

}
