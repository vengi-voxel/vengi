project(aidebug)
set(SRCS
	network/AINetwork.cpp network/AINetwork.h
	network/MessageSender.h network/MessageSender.cpp

	AIDebug.h AIDebug.cpp
)

engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} WINDOWED)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES imgui ai-shared network attrib)
