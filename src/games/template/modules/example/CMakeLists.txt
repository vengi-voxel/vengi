set(SRCS Example.h Example.cpp)
set(LIB template-example)

set(DEPENDENCIES)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES ${DEPENDENCIES})
target_include_directories(${LIB} PUBLIC ..)
