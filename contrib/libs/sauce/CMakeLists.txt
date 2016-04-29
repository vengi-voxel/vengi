set(SRCS
	dummy.cpp
)

engine_add_library(
	LIB sauce
	SRCS ${SRCS}
)
if (NOT SAUCE_FOUND)
	target_include_directories(sauce PUBLIC .)
	target_compile_options(sauce PUBLIC "-Wno-deprecated-declarations")
endif()
