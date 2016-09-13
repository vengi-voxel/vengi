set(SRCS
	dummy.cpp
)

engine_add_library(
	LIB selene
	SRCS ${SRCS}
)

if (NOT SELENE_FOUND)
	target_link_libraries(selene lua53)
endif()
