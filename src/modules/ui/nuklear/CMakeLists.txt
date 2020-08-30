set(SRCS
	NuklearApp.cpp NuklearApp.h
	NuklearNode.cpp NuklearNode.h
	Nuklear.h

	Console.cpp Console.h

	LUAUIApp.cpp LUAUIApp.h
	LUAFunctions.cpp LUAFunctions.h

	private/nuklear.h
)
set(FILES
	shared/font.ttf
)
engine_add_module(TARGET nuklear SRCS ${SRCS} FILES ${FILES} DEPENDENCIES app voxelrender)
