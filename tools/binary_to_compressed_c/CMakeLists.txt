project(binary_to_compressed_c)
set(SRCS
	binary_to_compressed_c.cpp
)

engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS})
