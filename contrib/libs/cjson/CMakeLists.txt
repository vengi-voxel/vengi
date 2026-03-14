set(SRCS
	cJSON.c
	cJSON.h
)
add_library(cjson STATIC ${SRCS})
target_include_directories(cjson PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
engine_target_optimize(cjson)
