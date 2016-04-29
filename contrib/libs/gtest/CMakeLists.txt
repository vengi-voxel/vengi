set(SRCS
	src/gtest-all.cc
)
set(DEFINES GTEST_HAS_EXCEPTIONS=0 GTEST_HAS_RTTI=0 GTEST_HAS_PTHREAD=0)
if (DARWIN)
	list(APPEND DEFINES GTEST_USE_OWN_TR1_TUPLE=1)
endif()

engine_add_library(
	LIB gtest
	SRCS ${SRCS}
	DEFINES ${DEFINES}
)
target_include_directories(gtest PUBLIC . include/)
