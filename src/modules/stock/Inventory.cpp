/**
 * @file
 */

#include "Inventory.h"

namespace stock {

Inventory::Inventory() {
	_containers.fill(Container());
}

bool Inventory::initContainer(uint8_t containerId, const ContainerShape& shape) {
	if (containerId >= maxContainers()) {
		return false;
	}
	Container& c = _containers[containerId];
	c.init(shape);
	return true;
}

void Inventory::clear() {
	for (int i = 0; i < maxContainers(); ++i) {
		Container& c = _containers[i];
		c.clear();
	}
}

bool Inventory::notifyRemove(const ItemPtr& item) {
	if (item == nullptr) {
		return false;
	}
	for (int i = 0; i < maxContainers(); ++i) {
		Container& c = _containers[i];
		if (c.notifyRemove(item)) {
			return true;
		}
	}
	return false;
}

bool Inventory::add(uint8_t containerId, const ItemPtr& item, uint8_t x, uint8_t y) {
	if (!item) {
		return false;
	}
	if (containerId >= maxContainers()) {
		return false;
	}
	Container& c = _containers[containerId];
	return c.add(item, x, y);
}

ItemPtr Inventory::remove(uint8_t containerId, uint8_t x, uint8_t y) {
	if (containerId >= maxContainers()) {
		return ItemPtr();
	}
	Container& c = _containers[containerId];
	return c.remove(x, y);
}

}
