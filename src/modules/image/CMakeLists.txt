set(SRCS
	Image.cpp Image.h
)
set(LIB image)
add_library(${LIB} ${SRCS})
target_link_libraries(${LIB} core io)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
