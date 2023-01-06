project(binary_to_compressed_c)
set(SRCS
	binary_to_compressed_c.cpp
)

engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} NOINSTALL)
target_compile_options(${PROJECT_NAME} PRIVATE $<$<CXX_COMPILER_ID:GNU>:-Wno-unused-but-set-variable>)
set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER tools)
