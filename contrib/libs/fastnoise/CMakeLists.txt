set(SRCS
	FastNoise.cpp
	FastNoise.h
)

engine_add_library(
	LIB fastnoise
	SRCS ${SRCS}
)
target_link_libraries(${LIB} glm)
