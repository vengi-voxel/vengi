set(SRCS
	dummy.cpp
)

engine_add_library(
	LIB boostdi
	SRCS ${SRCS}
)
if (NOT BOOSTDI_FOUND)
	target_compile_options(boostdi PUBLIC "-Wno-deprecated-declarations")
endif()
