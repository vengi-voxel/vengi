set(SRCS
	dearimgui/imgui.cpp dearimgui/imgui_draw.cpp
	dearimgui/imgui.h
	dearimgui/imgui_internal.h
	dearimgui/imconfig.h
	dearimgui/stb_rect_pack.h
	dearimgui/stb_textedit.h
	dearimgui/stb_truetype.h

	dearimgui/addons/imguinodegrapheditor/imguinodegrapheditor.h
	dearimgui/addons/imguinodegrapheditor/imguinodegrapheditor.cpp

	dearimgui/addons/imguihelper/imguihelper.h
	dearimgui/addons/imguihelper/imguihelper.cpp
)
set(LIB_NAME dearimgui)
engine_add_library(
	LIB ${LIB_NAME}
	SRCS ${SRCS}
)

set(ADDITIONAL_COMPILE_FLAGS)
check_c_compiler_flag("-Wimplicit-fallthrough=0" HAS_IMPLICIT_FALLTHROUGH)
if (HAS_IMPLICIT_FALLTHROUGH)
	set(ADDITIONAL_COMPILE_FLAGS "${ADDITIONAL_COMPILE_FLAGS} -Wimplicit-fallthrough=0")
endif()
set_target_properties(${LIB_NAME} PROPERTIES COMPILE_FLAGS "${ADDITIONAL_COMPILE_FLAGS}")

add_definitions(-DIMGUI_INCLUDE_IMGUI_USER_H -DIMGUI_INCLUDE_IMGUI_USER_INL)
target_include_directories(dearimgui PRIVATE dearimgui)
