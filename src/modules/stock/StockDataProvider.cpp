/**
 * @file
 */

#include "StockDataProvider.h"
#include "LUAStock.h"
#include "core/Log.h"

namespace stock {

StockDataProvider::StockDataProvider() {
	_itemData.fill(nullptr);
}

StockDataProvider::~StockDataProvider() {
	shutdown();
}

void StockDataProvider::shutdown() {
	reset();
}

void StockDataProvider::reset() {
	for (const auto* data : _itemData) {
		delete data;
	}
	for (const auto& entry : _containerDataMap) {
		delete entry.second;
	}
	_itemData.fill(nullptr);
	_containerDataMap.clear();
	_error = "";
}

bool StockDataProvider::init(const core::String& luaScript) {
	if (luaScript.empty()) {
		_error = "empty lua script given";
		return false;
	}
	_error = "";

	lua::LUA lua;
	luastock_setup(lua, this);
	if (!lua.load(luaScript)) {
		_error = lua.error();
		return false;
	}

	if (!lua.execute("init")) {
		_error = lua.error();
		return false;
	}

	return true;
}

ItemPtr StockDataProvider::createItem(ItemId itemId) {
	if (itemId > _itemData.size()) {
		Log::error("Invalid item id %i", (int)itemId);
		return ItemPtr();
	}
	const ItemData* data = itemData(itemId);
	if (data == nullptr) {
		Log::error("Could not find item for id %i", (int)itemId);
		return ItemPtr();
	}
	Log::trace("Create item with id %i", (int)itemId);
	return std::make_shared<Item>(*data);
}

bool StockDataProvider::addItemData(ItemData* data) {
	const ItemId id = data->id();
	if (itemData(id) != nullptr) {
		Log::error("Invalid item id %i - an entry with that id already exists", (int)id);
		return false;
	}
	_itemData[id] = data;
	return true;
}

const ItemData* StockDataProvider::itemData(ItemId itemId) const {
	if (itemId >= _itemData.size()) {
		Log::error("Invalid item id %i", (int)itemId);
		return nullptr;
	}
	const ItemData* data = _itemData[itemId];
	return data;
}

const ItemData* StockDataProvider::itemData(const core::String& name) const {
	for (const auto& i : _itemData) {
		if (i->name() == name) {
			return i;
		}
	}
	return nullptr;
}

bool StockDataProvider::addContainerData(ContainerData* data) {
	if (containerData(data->name) != nullptr) {
		Log::error("Invalid container id %s - an entry with that name already exists", data->name.c_str());
		return false;
	}
	auto it = std::find_if(_containerDataMap.begin(), _containerDataMap.end(),
		[data](const std::pair<core::String, ContainerData*> &t) -> bool {
			return t.second->id == data->id;
		}
	);
	if (it != _containerDataMap.end()) {
		Log::error("Invalid container id for %s - an entry with that id already exists", data->name.c_str());
		return false;
	}
	_containerDataMap[data->name] = data;
	return true;
}

const ContainerData* StockDataProvider::containerData(const core::String& name) const {
	auto i = _containerDataMap.find(name);
	if (i == _containerDataMap.end()) {
		Log::warn("Failed to get container with name '%s'", name.c_str());
		return nullptr;
	}
	return i->second;
}

}
