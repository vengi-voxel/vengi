/**
 * @file
 */

#include "LUAStock.h"
#include "commonlua/LUA.h"
#include "commonlua/LUAFunctions.h"
#include "stock/StockDataProvider.h"
#include "stock/Shape.h"
#include "core/StringUtil.h"

namespace stock {

static const char *luastock_providerid() {
	return "__global_provider";
}

static const char* luastock_metastock() {
	return "__meta_stock";
}

static const char* luastock_metacontainershape() {
	return "__meta_containershape";
}

static const char* luastock_metacontainer() {
	return "__meta_container";
}

static const char* luastock_metaitem() {
	return "__meta_item";
}

static const char* luastock_metaitemshape() {
	return "__meta_itemshape";
}

static StockDataProvider* luastock_getprovider(lua_State *s) {
	lua_getglobal(s, luastock_providerid());
	StockDataProvider *provider = (StockDataProvider *)lua_touserdata(s, -1);
	lua_pop(s, 1);
	return provider;
}

static void luastock_pushprovider(lua_State* s, StockDataProvider* provider) {
	lua_pushlightuserdata(s, provider);
	lua_setglobal(s, luastock_providerid());
}

static int luastock_create_container(lua_State * l) {
	StockDataProvider *stockDataProvider = luastock_getprovider(l);
	if (stockDataProvider == nullptr) {
		return lua::LUA::returnError(l, "Could not find global provider");
	}
	const uint8_t containerId = (uint8_t)luaL_checkinteger(l, 1);
	const char* containerName = luaL_checkstring(l, 2);
	ContainerData* containerData = new ContainerData();
	containerData->name = containerName;
	containerData->id = containerId;
	if (!stockDataProvider->addContainerData(containerData)) {
		delete containerData;
		return lua::LUA::returnError(l, "Could not add container with name: %s", containerName);
	}
	return clua_pushudata(l, containerData, luastock_metacontainer());
}

static ContainerData* luastock_tocontainer(lua_State * l, int n) {
	return *(ContainerData**)clua_getudata<ContainerData*>(l, n, luastock_metacontainer());
}

static int luastock_container_getshape(lua_State * l) {
	ContainerData *containerData = luastock_tocontainer(l, 1);
	return clua_pushudata(l, &containerData->shape, luastock_metacontainershape());
}

static int luastock_container_gc(lua_State * l) {
	// this is deleted in the StockDataProvider
	return 0;
}

static int luastock_container_tostring(lua_State * l) {
	const ContainerData *containerData = luastock_tocontainer(l, 1);
	lua_pushfstring(l, "container: %d (%s)", (int)containerData->id, containerData->name.c_str());
	return 1;
}

static int luastock_container_getname(lua_State * l) {
	const ContainerData *containerData = luastock_tocontainer(l, 1);
	lua_pushstring(l, containerData->name.c_str());
	return 1;
}

static int luastock_container_getid(lua_State * l) {
	const ContainerData *containerData = luastock_tocontainer(l, 1);
	lua_pushinteger(l, containerData->id);
	return 1;
}

static ContainerShape* luastock_tocontainershape(lua_State * l, int n) {
	return *(ContainerShape**)clua_getudata<ContainerShape*>(l, n, luastock_metacontainershape());
}

static int luastock_containershape_addrect(lua_State * l) {
	ContainerShape *containerShape = luastock_tocontainershape(l, 1);
	if (containerShape == nullptr) {
		return lua::LUA::returnError(l, "Expected container shape as first parameter");
	}
	const uint8_t x = luaL_checkinteger(l, 2);
	const uint8_t y = luaL_checkinteger(l, 3);
	const uint8_t w = luaL_checkinteger(l, 4);
	const uint8_t h = luaL_checkinteger(l, 5);
	containerShape->addRect(x, y, w, h);
	return 0;
}

static int luastock_containershape_gc(lua_State * l) {
	return 0;
}

static int luastock_containershape_tostring(lua_State * l) {
	const ContainerShape& containerShape = *luastock_tocontainershape(l, 1);
	lua_pushfstring(l, "container shape:\nsize: %d, free: %d", containerShape.size(), containerShape.free());
	return 1;
}

static int luastock_create_item(lua_State * l) {
	StockDataProvider *stockDataProvider = luastock_getprovider(l);
	if (stockDataProvider == nullptr) {
		return lua::LUA::returnError(l, "Could not find global provider");
	}
	const ItemId itemId = luaL_checkinteger(l, 1);
	const char *type = luaL_checkstring(l, 2);
	const char *name = luaL_optstring(l, 3, nullptr);
	const ItemType itemType = getItemType(type);
	if (itemType == ItemType::NONE) {
		return lua::LUA::returnError(l, "Unknown type given: %s", type);
	}

	ItemData* itemData = new ItemData(itemId, itemType);
	if (name != nullptr) {
		itemData->setName(name);
	}

	if (!stockDataProvider->addItemData(itemData)) {
		delete itemData;
		lua_pushnil(l);
		return 1;
	}
	return clua_pushudata(l, itemData, luastock_metaitem());
}

static ItemData* luastock_toitem(lua_State * l, int n) {
	return *(ItemData**)clua_getudata<ItemData*>(l, n, luastock_metaitem());
}

static int luastock_item_getshape(lua_State * l) {
	ItemData *itemData = luastock_toitem(l, 1);
	return clua_pushudata(l, &itemData->shape(), luastock_metaitemshape());
}

static int luastock_item_gc(lua_State * l) {
	// this is deleted in the StockDataProvider
	return 0;
}

static int luastock_pushitem(lua_State * l, ItemData* itemData) {
	return clua_pushudata(l, itemData, luastock_metaitem());
}

static int luastock_item_tostring(lua_State * l) {
	const ItemData *itemData = luastock_toitem(l, 1);
	lua_pushfstring(l, "item: %d (%s)", (int)itemData->id(), itemData->name());
	return 1;
}

static int luastock_item_getname(lua_State * l) {
	const ItemData *itemData = luastock_toitem(l, 1);
	lua_pushstring(l, itemData->name());
	return 1;
}

static int luastock_item_setname(lua_State * l) {
	ItemData *itemData = luastock_toitem(l, 1);
	itemData->setName(luaL_checkstring(l, 2));
	return luastock_pushitem(l, itemData);
}

static int luastock_item_addlabel(lua_State * l) {
	ItemData *itemData = luastock_toitem(l, 1);
	const char *key = luaL_checkstring(l, 2);
	const char *value = luaL_checkstring(l, 3);
	itemData->addLabel(key, value);
	return luastock_pushitem(l, itemData);
}

static int luastock_item_setsize(lua_State * l) {
	ItemData *itemData = luastock_toitem(l, 1);
	const int w = luaL_checkinteger(l, 2);
	const int h = luaL_checkinteger(l, 3);
	itemData->setSize(w, h);
	return luastock_pushitem(l, itemData);
}

static int luastock_item_getid(lua_State * l) {
	const ItemData *itemData = luastock_toitem(l, 1);
	lua_pushinteger(l, itemData->id());
	return 1;
}

static ItemShape* luastock_toitemshape(lua_State * l, int n) {
	return *(ItemShape**)clua_getudata<ItemShape*>(l, n, luastock_metaitemshape());
}

static int luastock_itemshape_addrect(lua_State * l) {
	ItemShape *itemShape = luastock_toitemshape(l, 1);
	const uint8_t x = luaL_checkinteger(l, 2);
	const uint8_t y = luaL_checkinteger(l, 3);
	const uint8_t w = luaL_checkinteger(l, 4);
	const uint8_t h = luaL_checkinteger(l, 5);
	itemShape->addRect(x, y, w, h);
	return 0;
}

static int luastock_itemshape_gc(lua_State * l) {
	return 0;
}

static int luastock_itemshape_tostring(lua_State * l) {
	const ItemShape& itemShape = *luastock_toitemshape(l, 1);
	lua_pushfstring(l, "item shape:\nw: %d, h: %d", itemShape.width(), itemShape.height());
	return 1;
}

void luastock_setup(lua_State* s, StockDataProvider* provider) {
	static const luaL_Reg itemFuncs[] = {
		{"name",       luastock_item_getname},
		{"setName",    luastock_item_setname},
		{"shape",      luastock_item_getshape},
		{"setSize",    luastock_item_setsize},
		{"addLabel",   luastock_item_addlabel},
		{"id",         luastock_item_getid},
		{"__gc",       luastock_item_gc},
		{"__tostring", luastock_item_tostring}
	};
	clua_registerfuncs(s, itemFuncs, luastock_metaitem());

	static const luaL_Reg containerFuncs[] = {
		{"name",       luastock_container_getname},
		{"shape",      luastock_container_getshape},
		{"id",         luastock_container_getid},
		{"__gc",       luastock_container_gc},
		{"__tostring", luastock_container_tostring}
	};
	clua_registerfuncs(s, containerFuncs, luastock_metacontainer());

	static const luaL_Reg containerShapeFuncs[] = {
		{"addRect",    luastock_containershape_addrect},
		{"__gc",       luastock_containershape_gc},
		{"__tostring", luastock_containershape_tostring}
	};
	clua_registerfuncs(s, containerShapeFuncs, luastock_metacontainershape());

	static const luaL_Reg itemShapeFuncs[] = {
		{"addRect",    luastock_itemshape_addrect},
		{"__gc",       luastock_itemshape_gc},
		{"__tostring", luastock_itemshape_tostring}
	};
	clua_registerfuncs(s, itemShapeFuncs, luastock_metaitemshape());

	static const luaL_Reg stockFuncs[] = {
		{"createItem",      luastock_create_item},
		{"createContainer", luastock_create_container},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, stockFuncs, luastock_metastock());
	lua_setglobal(s, "stock");

	luastock_pushprovider(s, provider);

	clua_register(s);
	clua_mathregister(s);
}

}

