/**
 * @file
 */

#include "ItemProvider.h"
#include "LUAFunctions.h"

namespace stock {

ItemProvider::ItemProvider() {
	_itemData.fill(nullptr);
}

ItemProvider::~ItemProvider() {
	shutdown();
}

void ItemProvider::shutdown() {
	reset();
}

bool ItemProvider::init(const std::string& luaScript) {
	if (luaScript.empty()) {
		_error = "empty lua script given";
		return false;
	}
	_error = "";

	lua::LUA lua;
	luaL_Reg createItem = { "createItem", luaCreateItemData };
	luaL_Reg eof = { nullptr, nullptr };
	luaL_Reg funcs[] = { createItem, eof };

	lua::LUAType item = lua.registerType("Item");
	item.addFunction("getName", luaItemDataGetName);
	item.addFunction("getShape", luaItemDataGetShape);
	item.addFunction("getId", luaItemDataGetId);
	item.addFunction("__gc", luaItemDataGC);
	item.addFunction("__tostring", luaItemDataToString);

	lua::LUAType shape = lua.registerType("Shape");
	shape.addFunction("addRect", luaItemDataShapeAddRect);
	shape.addFunction("__gc", luaItemDataShapeGC);
	shape.addFunction("__tostring", luaItemDataShapeToString);

	lua.reg("item", funcs);

	if (!lua.load(luaScript)) {
		_error = lua.error();
		return false;
	}

	// loads all the attributes
	lua.newGlobalData<ItemProvider>("Provider", this);
	if (!lua.execute("init")) {
		_error = lua.error();
		return false;
	}

	Log::info("loaded %i items", (int)_itemData.size());

	return true;
}

ItemPtr ItemProvider::createItem(ItemId itemId) {
	if (itemId > _itemData.size()) {
		Log::error("Invalid item id %i", (int)itemId);
		return ItemPtr();
	}
	const ItemData* data = getItemData(itemId);
	if (data == nullptr) {
		Log::error("Could not find item for id %i", (int)itemId);
		return ItemPtr();
	}
	Log::trace("Create item with id %i", (int)itemId);
	return std::make_shared<Item>(*data);
}

bool ItemProvider::addItemData(const ItemData* itemData) {
	const ItemId id = itemData->id();
	// an entry with that id already exists
	if (getItemData(id) != nullptr) {
		return false;
	}
	_itemData[id] = itemData;
	return true;
}

const ItemData* ItemProvider::getItemData(ItemId itemId) const {
	if (itemId > _itemData.size()) {
		Log::error("Invalid item id %i", (int)itemId);
		return nullptr;
	}
	if (itemId > _itemData.size()) {
		return nullptr;
	}
	const ItemData* data = _itemData[itemId];
	return data;
}

}
