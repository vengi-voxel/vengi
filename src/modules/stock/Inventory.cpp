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

bool Inventory::notifyRemove(Item* item) {
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

bool Inventory::add(uint8_t containerId, Item* item, uint8_t x, uint8_t y) {
	if (item == nullptr) {
		return false;
	}
	if (containerId >= maxContainers()) {
		return false;
	}
	Container& c = _containers[containerId];
	return c.add(item, x, y);
}

Item* Inventory::remove(uint8_t containerId, uint8_t x, uint8_t y) {
	if (containerId >= maxContainers()) {
		return nullptr;
	}
	Container& c = _containers[containerId];
	return c.remove(x, y);
}

}
