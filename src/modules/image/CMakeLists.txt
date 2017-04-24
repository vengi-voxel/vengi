set(SRCS
	Image.cpp Image.h
)
set(LIB image)
add_library(${LIB} ${SRCS})
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES core io)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
