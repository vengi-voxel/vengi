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
add_library(${LIB} ${SRCS})
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES core attrib network cooldown commonlua)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/StockTest.cpp
	tests/ShapeTest.cpp
	tests/InventoryTest.cpp
	tests/ContainerTest.cpp
	tests/StockDataProviderTest.cpp
)
gtest_suite_deps(tests ${LIB})
