/**
 * @file
 */

#include "StockDataProvider.h"
#include "LUAFunctions.h"
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

bool StockDataProvider::init(const std::string& luaScript) {
	if (luaScript.empty()) {
		_error = "empty lua script given";
		return false;
	}
	_error = "";

	lua::LUA lua;
	luaL_Reg createItem = { "createItem", luaCreateItemData };
	luaL_Reg createContainer = { "createContainer", luaCreateContainer };
	luaL_Reg eof = { nullptr, nullptr };
	luaL_Reg funcs[] = { createItem, createContainer, eof };

	lua::LUAType item = lua.registerType("Item");
	item.addFunction("name", luaItemDataGetName);
	item.addFunction("setName", luaItemDataSetName);
	item.addFunction("shape", luaItemDataGetShape);
	item.addFunction("setSize", luaItemDataSetSize);
	item.addFunction("addLabel", luaItemDataAddLabel);
	item.addFunction("id", luaItemDataGetId);
	item.addFunction("__gc", luaItemDataGC);
	item.addFunction("__tostring", luaItemDataToString);

	lua::LUAType container = lua.registerType("Container");
	container.addFunction("name", luaContainerDataGetName);
	container.addFunction("shape", luaContainerDataGetShape);
	container.addFunction("id", luaContainerDataGetId);
	container.addFunction("__gc", luaContainerDataGC);
	container.addFunction("__tostring", luaContainerDataToString);

	lua::LUAType containerShape = lua.registerType("ContainerShape");
	containerShape.addFunction("addRect", luaContainerDataShapeAddRect);
	containerShape.addFunction("__gc", luaContainerDataShapeGC);
	containerShape.addFunction("__tostring", luaContainerDataShapeToString);

	lua::LUAType itemShape = lua.registerType("ItemShape");
	itemShape.addFunction("addRect", luaItemDataShapeAddRect);
	itemShape.addFunction("__gc", luaItemDataShapeGC);
	itemShape.addFunction("__tostring", luaItemDataShapeToString);

	lua.reg("stock", funcs);

	if (!lua.load(luaScript)) {
		_error = lua.error();
		return false;
	}

	// loads all the attributes
	lua.newGlobalData<StockDataProvider>("Provider", this);
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

const ItemData* StockDataProvider::itemData(const std::string& name) const {
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
		[data](const std::pair<std::string, ContainerData*> &t) -> bool {
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

const ContainerData* StockDataProvider::containerData(const std::string& name) const {
	auto i = _containerDataMap.find(name);
	if (i == _containerDataMap.end()) {
		Log::warn("Failed to get container with name '%s'", name.c_str());
		return nullptr;
	}
	return i->second;
}

}
