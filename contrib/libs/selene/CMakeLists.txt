set(LIB selene)
set(SRCS
	dummy.cpp
)

engine_add_library(
	LIB ${LIB}
	SRCS ${SRCS}
)

if (NOT SELENE_FOUND)
	target_link_libraries(${LIB} lua53)
	if (NOT MSVC)
		set_target_properties(${LIB} PROPERTIES COMPILE_FLAGS "-Wno-unused-variable")
	endif()
endif()
