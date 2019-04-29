set(SRCS
	Image.cpp Image.h
)
set(LIB image)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES core)
if (USE_CLANG)
	target_compile_options(${LIB} PRIVATE -Wno-unused-function)
endif()
