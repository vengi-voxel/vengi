set(SRCS
	Container.h Container.cpp
	Inventory.h Inventory.cpp
	ItemData.h ItemData.cpp
	Item.h Item.cpp
	Shape.h Shape.cpp
	Stock.h Stock.cpp
	ItemProvider.h ItemProvider.cpp
)

set(LIB stock)
add_library(${LIB} ${SRCS})
target_link_libraries(${LIB} core attrib network cooldown commonlua)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})

gtest_suite_files(tests
	tests/StockTest.cpp
	tests/ShapeTest.cpp
	tests/InventoryTest.cpp
	tests/ContainerTest.cpp
	tests/ItemProviderTest.cpp
)
gtest_suite_deps(tests ${LIB})
