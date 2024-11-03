set(SRCS
	JSON.h JSON.cpp
)

set(LIB json)
set(LIBS core)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES ${LIBS})
