set(SRCS
	IMGUIApp.cpp IMGUIApp.h
	IMGUI.h
	IMGUIInternal.h
	IMGUIAddons.h
)
set(LIB imgui)
add_library(${LIB} ${SRCS})
generate_shaders(${LIB} texture)
target_link_libraries(${LIB} core io video dearimgui)
set_target_properties(${LIB} PROPERTIES FOLDER ${LIB})
target_include_directories(${LIB} PUBLIC .)
