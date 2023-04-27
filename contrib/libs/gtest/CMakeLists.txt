set(SRCS
	src/gtest-all.cc
)

engine_add_library(
	LIB gtest
	SRCS ${SRCS}
	DEFINES ${DEFINES}
)
if (NOT GTEST_FOUND)
	target_include_directories(gtest PUBLIC . include/)
endif()
