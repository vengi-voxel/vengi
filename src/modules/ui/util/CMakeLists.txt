set(LIB uiutil)

set(SRCS
	Window.h Window.cpp
	WindowStack.h WindowStack.cpp
)

engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES ui audio)
