set(SRCS
	src/component_storage.cpp
	src/demangle_type_name.cpp
	src/injector_storage.cpp
	src/normalized_component_storage.cpp
	src/semistatic_map.cpp
)

engine_add_library(
	LIB fruit
	SRCS ${SRCS}
)
target_include_directories(fruit PUBLIC include/)
