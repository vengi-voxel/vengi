/**
 * @file
 */

#pragma once

#include "Container.h"
#include <array>

namespace stock {

// TODO: container move operations with filters
//       if might e.g. cost currency to move from one container to another
/**
 * @ingroup Stock
 */
class Inventory {
private:
	std::array<Container, 16>  _containers;
public:
	Inventory();

	int maxContainers() const;

	bool initContainer(uint8_t containerId, const ContainerShape& shape);

	void clear();

	/**
	 * @brief Remove the item from the highest order Container instances
	 * until all of the linked items are removed.
	 */
	bool notifyRemove(const ItemPtr& item);

	bool add(uint8_t containerId, const ItemPtr& item, uint8_t x, uint8_t y);

	ItemPtr remove(uint8_t containerId, uint8_t x, uint8_t y);

	const Container* container(uint8_t containerId) const;
};

inline const Container* Inventory::container(uint8_t containerId) const {
	if (containerId >= maxContainers()) {
		return nullptr;
	}
	const Container& c = _containers[containerId];
	return &c;
}

inline int Inventory::maxContainers() const {
	return _containers.size();
}

}
