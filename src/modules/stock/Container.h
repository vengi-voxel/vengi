/**
 * @file
 */

#pragma once

#include "Shape.h"
#include "ItemData.h"

namespace stock {

class Item;
typedef std::shared_ptr<Item> ItemPtr;

/**
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

	void init(const ContainerShape& shape);

	void clear();

	const ContainerItems& items() const;

	bool hasItemOfType(const ItemType& itemType) const;

	/**
	 * @brief Compute the overall item count
	 */
	size_t itemCount() const;

	bool findSpace(const ItemPtr& item, uint8_t& x, uint8_t& y) const;

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

	/** each item can only be in here once */
	static constexpr uint32_t Unique     = 1 << 0;
	/** only a single item can be in this container */
	static constexpr uint32_t Single     = 1 << 1;
	/** a scrollable container can hold as many items as wanted */
	static constexpr uint32_t Scrollable = 1 << 2;

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
