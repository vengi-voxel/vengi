set(SRCS dummy.cpp)
engine_add_library(LIB glm SRCS ${SRCS})
target_compile_definitions(glm PUBLIC GLM_FORCE_CXX14)
target_precompile_headers(glm PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/pch.h>")
