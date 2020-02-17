/**
 * @file
 */

#pragma once

#include "Shape.h"
#include "ItemData.h"
#include <vector>

namespace stock {

class Item;
typedef std::shared_ptr<Item> ItemPtr;

/**
 * @brief A container is a collection of items. They are packed into a @c ContainerItem.
 * Each Container instance has a @c ContainerShape assigned which defines the valid area to place
 * items at.
 * @ingroup Stock
 */
class Container {
public:
	struct ContainerItem {
		ItemPtr item;
		uint8_t x;
		uint8_t y;
	};
	typedef std::vector<ContainerItem> ContainerItems;

	/** each item can only be in here once */
	static constexpr uint32_t Unique     = 1 << 0;
	/** only a single item can be in this container */
	static constexpr uint32_t Single     = 1 << 1;
	/** a scrollable container can hold as many items as wanted */
	static constexpr uint32_t Scrollable = 1 << 2;

	/**
	 * @param[in] flags Bitmask of flags to control the behavior of the container
	 */
	void init(const ContainerShape& shape, uint32_t flags = 0u);

	void clear();

	const ContainerItems& items() const;

	bool hasItemOfType(const ItemType& itemType) const;

	/**
	 * @brief Compute the overall item count
	 */
	size_t itemCount() const;

	/**
	 * @brief Find a free location in the container to place the given item at
	 * @param[out] x The x location to place the item
	 * @param[out] y The y location to place the item
	 * @return @c true if a free location was found, @c false otherwise
	 */
	bool findSpace(const ItemPtr& item, uint8_t& x, uint8_t& y) const;

	/**
	 * @brief Check whether the given item can be added to the specified location in the container
	 * @return @c true if the placement would work, @c false if not
	 */
	bool canAdd(const ItemPtr& item, uint8_t x, uint8_t y) const;

	bool add(const ItemPtr& item, uint8_t x, uint8_t y);

	bool add(const ItemPtr& item);

	bool notifyRemove(const ItemPtr& item);

	ItemPtr remove(uint8_t x, uint8_t y);

	ItemPtr get(uint8_t x, uint8_t y) const;

	int size() const;

	int free() const;
private:
	auto findById(ItemId id) const;

	auto findByType(const ItemType& type) const;

	ContainerShape _shape;
	uint32_t _flags = 0u;
	ContainerItems _items;
};

inline int Container::size() const {
	return _shape.size();
}

inline int Container::free() const {
	return _shape.free();
}

inline void Container::clear() {
	_items.clear();
}

inline size_t Container::itemCount() const {
	return items().size();
}

inline const Container::ContainerItems& Container::items() const {
	return _items;
}

}
