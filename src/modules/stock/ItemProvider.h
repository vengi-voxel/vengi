/**
 * @file
 */

#pragma once

#include "Item.h"
#include <array>

namespace stock {

class ItemProvider {
public:
	typedef std::array<const ItemData*, 4096> ItemDataContainer;

	ItemProvider();
	~ItemProvider();

	/**
	 * @param luaScript The lua script string to load
	 * @return @c true on success, @c false if an error occurred. In case of an error,
	 * you can call error() to get more information about it.
	 * @note this can be called multiple times. But beware, if a @c Item with the same
	 * type already exists, it will just be overwritten-
	 */
	bool init(const std::string& luaScript);

	void shutdown();

	/**
	 * @brief Removes all known item data entries from previous init() calls
	 */
	void reset();

	/**
	 * @return Immutable list of ItemData instances that were already parsed.
	 */
	const ItemDataContainer& itemData() const;

	/**
	 * @note Takes ownership of the ItemData instance and will delete it on reset()
	 * @return If this returns @c false, the instance will not get managed by this class and must be freed manually.
	 */
	bool addItemData(const ItemData* itemData);

	const ItemData* getItemData(ItemId itemId) const;

	/**
	 * @brief Creates a new item.
	 */
	Item* createItem(ItemId itemId);

	/**
	 * @return The last error that occurred in an init() call
	 */
	const std::string& error() const;

private:
	ItemDataContainer _itemData;
	std::string _error;
};

inline const std::string& ItemProvider::error() const {
	return _error;
}

inline void ItemProvider::reset() {
	for (const ItemData* itemData : _itemData) {
		ItemData* deleteItemData = const_cast<ItemData*>(itemData);
		delete deleteItemData;
	}
	_itemData.fill(nullptr);
	_error = "";
}

inline const ItemProvider::ItemDataContainer& ItemProvider::itemData() const {
	return _itemData;
}


}
