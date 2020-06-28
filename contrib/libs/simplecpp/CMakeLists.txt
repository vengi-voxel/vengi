set(SRCS simplecpp.cpp simplecpp.h)
engine_add_library(LIB simplecpp SRCS ${SRCS})
if(MSVC)
	set_target_properties(simplecpp PROPERTIES COMPILE_FLAGS "/wd4005") # 'NOMINMAX': macro redefinition
else()
	set_target_properties(simplecpp PROPERTIES COMPILE_FLAGS "-fexceptions")
endif()
