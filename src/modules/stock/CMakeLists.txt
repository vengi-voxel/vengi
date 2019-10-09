set(SRCS
	Container.h Container.cpp
	Inventory.h Inventory.cpp
	ItemData.h ItemData.cpp
	Item.h Item.cpp
	Shape.h Shape.cpp
	Stock.h Stock.cpp
	StockDataProvider.h StockDataProvider.cpp
)
set(LUA_SRCS
	stock.lua
)
set(LIB stock)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} LUA_SRCS ${LUA_SRCS} DEPENDENCIES attrib cooldown)

gtest_suite_sources(tests
	tests/StockTest.cpp
	tests/ShapeTest.cpp
	tests/InventoryTest.cpp
	tests/ContainerTest.cpp
	tests/StockDataProviderTest.cpp
)
gtest_suite_deps(tests ${LIB})
