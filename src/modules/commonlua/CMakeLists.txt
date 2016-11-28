set(SRCS
	LUA.h LUA.cpp
	LUAFunctions.h LUAFunctions.cpp
)
set(LIB commonlua)
add_library(${LIB} ${SRCS})
target_link_libraries(${LIB} core selene)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
