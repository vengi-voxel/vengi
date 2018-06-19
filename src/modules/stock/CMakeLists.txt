set(SRCS
	Container.h Container.cpp
	Inventory.h Inventory.cpp
	ItemData.h ItemData.cpp
	Item.h Item.cpp
	Shape.h Shape.cpp
	Stock.h Stock.cpp
	StockDataProvider.h StockDataProvider.cpp
)

set(LIB stock)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES core attrib network cooldown commonlua)

gtest_suite_sources(tests
	tests/StockTest.cpp
	tests/ShapeTest.cpp
	tests/InventoryTest.cpp
	tests/ContainerTest.cpp
	tests/StockDataProviderTest.cpp
)
gtest_suite_deps(tests ${LIB})
