set(SRCS
	dearimgui/imgui.cpp dearimgui/imgui_draw.cpp
	dearimgui/imgui_widgets.cpp
	dearimgui/imgui_stdlib.cpp dearimgui/imgui_stdlib.h
	dearimgui/imgui.h
	dearimgui/imgui_internal.h
	dearimgui/imconfig.h
	dearimgui/imstb_rectpack.h
	dearimgui/imstb_textedit.h
	dearimgui/imstb_truetype.h
)
set(LIB_NAME dearimgui)
engine_add_library(
	LIB ${LIB_NAME}
	SRCS ${SRCS}
)

if (APPLE)
	target_link_libraries(${LIB_NAME} PRIVATE "-framework Carbon")
endif()

set(ADDITIONAL_COMPILE_FLAGS)
check_c_compiler_flag("-Wimplicit-fallthrough=0" HAS_IMPLICIT_FALLTHROUGH)
if (HAS_IMPLICIT_FALLTHROUGH)
	set(ADDITIONAL_COMPILE_FLAGS "${ADDITIONAL_COMPILE_FLAGS} -Wimplicit-fallthrough=0")
endif()
set_target_properties(${LIB_NAME} PROPERTIES COMPILE_FLAGS "${ADDITIONAL_COMPILE_FLAGS}")

target_include_directories(dearimgui PRIVATE dearimgui)
