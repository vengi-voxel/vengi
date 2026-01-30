set(SRCS
	tinyxml2.cpp
	tinyxml2.h
)
add_library(tinyxml2 STATIC ${SRCS})
target_include_directories(tinyxml2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
engine_target_optimize(tinyxml2)
