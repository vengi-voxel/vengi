/**
 * @file
 */

#pragma once

#include "commonlua/LUA.h"
#include "ItemProvider.h"
#include "core/String.h"

namespace stock {

static ItemProvider* luaGetItemProvider(lua_State * l) {
	return lua::LUA::globalData<ItemProvider>(l, "Provider");
}

static int luaCreateItemData(lua_State * l) {
	ItemProvider *itemProvider = luaGetItemProvider(l);
	const ItemId itemId = luaL_checkinteger(l, 1);
	const char *type = luaL_checkstring(l, 2);
	const ItemType itemType = getItemType(type);
	if (itemType == ItemType::NONE) {
		const std::string& error = core::string::format("Unknown type given: %s", type);
		lua::LUA::returnError(l, error);
	}

	ItemData* udata = lua::LUA::newUserdata<ItemData>(l, "Item", new ItemData(itemId, itemType));
	itemProvider->addItemData(udata);
	return 1;
}

static ItemData* luaGetItemData(lua_State * l, int n) {
	return lua::LUA::userData<ItemData>(l, n, "Item");
}

static int luaItemDataGetShape(lua_State * l) {
	ItemData *itemData = luaGetItemData(l, 1);
	lua::LUA::newUserdata<ItemShape>(l, "Shape", &itemData->shape());
	return 1;
}

static int luaItemDataGC(lua_State * l) {
	// this is deleted in the ItemProvider
	return 0;
}

static int luaItemDataToString(lua_State * l) {
	const ItemData *itemData = luaGetItemData(l, 1);
	lua_pushfstring(l, "item: %d (%s)", (int)itemData->id(), itemData->name());
	return 1;
}

static int luaItemDataGetName(lua_State * l) {
	const ItemData *ctx = luaGetItemData(l, 1);
	lua_pushstring(l, ctx->name());
	return 1;
}

static int luaItemDataGetId(lua_State * l) {
	const ItemData *ctx = luaGetItemData(l, 1);
	lua_pushinteger(l, ctx->id());
	return 1;
}

static ItemShape* luaGetItemDataShape(lua_State * l, int n) {
	return lua::LUA::userData<ItemShape>(l, n, "Shape");
}

static int luaItemDataShapeAddRect(lua_State * l) {
	ItemShape *itemShape = luaGetItemDataShape(l, 1);
	const uint8_t x = luaL_checkinteger(l, 2);
	const uint8_t y = luaL_checkinteger(l, 3);
	const uint8_t w = luaL_checkinteger(l, 4);
	const uint8_t h = luaL_checkinteger(l, 5);
	itemShape->addRect(x, y, w, h);
	return 0;
}

static int luaItemDataShapeGC(lua_State * l) {
	return 0;
}

static int luaItemDataShapeToString(lua_State * l) {
	const ItemShape *itemShape = luaGetItemDataShape(l, 1);
	lua_pushfstring(l, "shape:\n%s", core::string::bits((ItemShapeType)*itemShape, ItemMaxWidth).c_str());
	return 1;
}

}
