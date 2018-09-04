set(SRCS
	dearimgui/imgui.cpp dearimgui/imgui_draw.cpp
	dearimgui/imgui_widgets.cpp
	dearimgui/imgui.h
	dearimgui/imgui_internal.h
	dearimgui/imconfig.h
	dearimgui/stb_rect_pack.h
	dearimgui/stb_textedit.h
	dearimgui/stb_truetype.h
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

target_include_directories(dearimgui PRIVATE dearimgui)
