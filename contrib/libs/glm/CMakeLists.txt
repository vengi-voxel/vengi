set(SRCS dummy.cpp)
engine_add_library(LIB glm SRCS ${SRCS})
target_compile_definitions(glm PUBLIC GLM_FORCE_ALIGNED_GENTYPES GLM_FORCE_CXX14)
