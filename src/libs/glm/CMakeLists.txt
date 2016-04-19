set(SRCS glm/detail/glm.cpp)
engine_add_library(LIB glm SRCS ${SRCS} GCCCFLAGS "-Wno-shadow")
