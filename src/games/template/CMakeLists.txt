project(template)
add_subdirectory(modules)

set(SRCS
	Template.h Template.cpp
)

#set(FILES ${PROJECT_NAME}/${PROJECT_NAME}-keybindings.cfg)
set(FILES)
set(DEPENDENCIES template-example imgui)
engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} WINDOWED)
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES ${DEPENDENCIES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} modules)
