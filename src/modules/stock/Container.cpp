/**
 * @file
 */

#include "Container.h"
#include "Item.h"
#include <algorithm>

namespace stock {

void Container::init(const ContainerShape& shape) {
	_shape = shape;
	_items.reserve(64);
}

bool Container::canAdd(const Item* item, uint8_t x, uint8_t y) const {
	if (item == nullptr) {
		return false;
	}
	if ((_flags & Single) != 0 && !items().empty()) {
		return false;
	}
	if ((_flags & Unique) != 0 && hasItemOfType(item->type())) {
		return false;
	}
	if ((_flags & Scrollable) != 0) {
		return true;
	}
	if (!_shape.isFree(item->shape(), x, y)) {
		return false;
	}
	return true;
}

bool Container::add(Item* item) {
	uint8_t x;
	uint8_t y;
	if (!findSpace(item, x, y)) {
		return false;
	}
	if (!canAdd(item, x, y)) {
		return false;
	}
	return add(item, x, y);
}

auto Container::findById(ItemId id) const {
	return std::find_if(_items.begin(), _items.end(), [id] (const ContainerItem& item) { return item.item->id() == id; });
}

auto Container::findByType(const ItemType& type) const {
	return std::find_if(_items.begin(), _items.end(), [type] (const ContainerItem& item) { return item.item->type() == type; });
}

bool Container::hasItemOfType(const ItemType& itemType) const {
	return findByType(itemType) != _items.end();
}

bool Container::add(Item* item, uint8_t x, uint8_t y) {
	if (!canAdd(item, x, y)) {
		return false;
	}
	const ContainerItem ci = {item, x, y};
	_items.push_back(ci);
	_shape.addShape(static_cast<ItemShapeType>(item->shape()), x, y);
	return true;
}

bool Container::notifyRemove(Item* item) {
	const ItemId id = item->id();
	auto i = findById(id);
	if (i == _items.end()) {
		return false;
	}
	_shape.removeShape(static_cast<ItemShapeType>(item->shape()), i->x, i->y);
	_items.erase(i);
	return true;
}

Item* Container::remove(uint8_t x, uint8_t y) {
	Item* item = get(x, y);
	if (item == nullptr) {
		return nullptr;
	}
	if (!notifyRemove(item)) {
		return nullptr;
	}
	return item;
}

Item* Container::get(uint8_t x, uint8_t y) const {
	if (!_shape.isInShape(x, y)) {
		return nullptr;
	}
	if ((_flags & Single) != 0) {
		if (_items.empty()) {
			return nullptr;
		}
		return _items.front().item;
	}
	for (const ContainerItem& item : _items) {
		const ItemShape& shape = item.item->shape();
		if (shape.isInShape(x - item.x, y - y)) {
			return item.item;
		}
	}
	return nullptr;
}

bool Container::findSpace(const Item* item, uint8_t& targetX, uint8_t& targetY) const {
	// always fits into scrollable container
	if ((_flags & Scrollable) != 0) {
		targetX = targetY = 0u;
		return true;
	}

	// there is already an item.
	if ((_flags & Single) != 0 && !_items.empty()) {
		return false;
	}
	for (uint8_t y = 0; y < ContainerMaxHeight; ++y) {
		for (uint8_t x = 0; x < ContainerMaxWidth; ++x) {
			if (!canAdd(item, x, y)) {
				continue;
			}
			targetX = x;
			targetY = y;
			return true;
		}
	}

	return false;
}

}
