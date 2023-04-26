add_library(pch STATIC dummy.cpp)
set_target_properties(pch PROPERTIES LANGUAGE CXX)
target_precompile_headers(pch PUBLIC
	"$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/pch.h>"
)
