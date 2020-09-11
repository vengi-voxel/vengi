/**
 * @file
 */

#pragma once

#include "commonlua/LUA.h"

namespace stock {

class StockDataProvider;

extern void luastock_setup(lua_State* s, StockDataProvider* provider);

}
