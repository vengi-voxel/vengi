/**
 * @file
 */

#pragma once

#include "ContainerData.h"
#include "Item.h"
#include "core/collection/Array.h"
#include <memory>
#include <unordered_map>

namespace stock {

/**
 * @ingroup Stock
 */
class StockDataProvider {
public:
	typedef core::Array<const ItemData*, 4096> ItemDataContainer;
	typedef std::unordered_map<std::string, ContainerData*> ContainerDataMap;

	StockDataProvider();
	~StockDataProvider();

	/**
	 * @param luaScript The lua script string to load
	 * @return @c true on success, @c false if an error occurred. In case of an error,
	 * you can call error() to get more information about it.
	 * @note this can be called multiple times. But beware, if a @c Item with the same
	 * type already exists, it will just be overwritten-
	 */
	bool init(const core::String& luaScript);

	void shutdown();

	/**
	 * @brief Removes all known item data entries from previous init() calls
	 */
	void reset();

	/**
	 * @note Takes ownership of the @c ItemData instance and will delete it on @c reset()
	 * @return If this returns @c false, the instance will not get managed by this class and must be freed manually.
	 */
	bool addItemData(ItemData* itemData);

	/**
	 * O(1)
	 */
	const ItemData* itemData(ItemId itemId) const;

	/**
	 * O(n)
	 */
	const ItemData* itemData(const core::String& name) const;

	const ItemDataContainer& items() const;

	/**
	 * @note Takes ownership of the @c ContainerData instance and will delete it on @c reset()
	 * @return If this returns @c false, the instance will not get managed by this class and must be freed manually.
	 */
	bool addContainerData(ContainerData* containerData);

	const ContainerData* containerData(const core::String& name) const;

	const ContainerDataMap& containers() const;

	/**
	 * @brief Creates a new item.
	 */
	ItemPtr createItem(ItemId itemId);

	/**
	 * @return The last error that occurred in an init() call
	 */
	const core::String& error() const;

private:
	ItemDataContainer _itemData;
	ContainerDataMap _containerDataMap;
	core::String _error;
};

inline const StockDataProvider::ItemDataContainer& StockDataProvider::items() const {
	return _itemData;
}

inline const StockDataProvider::ContainerDataMap& StockDataProvider::containers() const {
	return _containerDataMap;
}

inline const core::String& StockDataProvider::error() const {
	return _error;
}

typedef std::shared_ptr<StockDataProvider> StockDataProviderPtr;

}
