set(SRCS
	LUA.h LUA.cpp
)
set(LIB commonlua)
add_library(${LIB} ${SRCS})
target_link_libraries(${LIB} core selene)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
