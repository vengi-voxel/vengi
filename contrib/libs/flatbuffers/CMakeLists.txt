set(SRCS
	dummy.cpp
	flatbuffers/flatbuffers.h
	flatbuffers/hash.h
	flatbuffers/idl.h
	flatbuffers/util.h
)

engine_add_library(
	LIB flatbuffers
	SRCS ${SRCS}
)
if (NOT FLATBUFFERS_FOUND)
	target_include_directories(flatbuffers PUBLIC .)
endif()