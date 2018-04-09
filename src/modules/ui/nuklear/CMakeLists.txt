set(SRCS
	NuklearApp.cpp NuklearApp.h
	Nuklear.h

	Console.cpp Console.h

	LUAUIApp.cpp LUAUIApp.h
	LUAFunctions.cpp LUAFunctions.h

	private/nuklear.h
)
set(LIB nuklear)
add_library(${LIB} ${SRCS})
engine_target_link_libraries(TARGET ${LIB} DEPENDENCIES core io render util commonlua)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
