set(LIB imgui)

set(SRCS
	Console.cpp Console.h
	FileDialog.cpp FileDialog.h
	IconsFontAwesome5.h
	IconsForkAwesome.h
	IMGUI.cpp IMGUI.h
	IMGUIApp.cpp IMGUIApp.h
	IMGUIInternal.h
	IMGUIStyle.cpp IMGUIStyle.h
	Notify.cpp Notify.h
	ScopedStyle.h
	TextEditor.h TextEditor.cpp
	Window.h Window.cpp
	WindowStack.h WindowStack.cpp
)

set(GEN_DIR ${GENERATE_DIR}/${LIB})
file(MAKE_DIRECTORY ${GEN_DIR})

macro(imgui_build_font NAME TTF_FILE)
	set(OUTPUT_FILE ${GEN_DIR}/${NAME}.h)
	set(INPUT_FILE ${ROOT_DIR}/data/${LIB}/${TTF_FILE})
	add_custom_command(
		OUTPUT ${OUTPUT_FILE}
		COMMAND
			binary_to_compressed_c
			${INPUT_FILE}
			${NAME} > ${OUTPUT_FILE}
		DEPENDS ${INPUT_FILE} binary_to_compressed_c
		WORKING_DIRECTORY ${GEN_DIR}
		COMMENT "Generate header for ${TTF_FILE} in ${GEN_DIR}"
		VERBATIM
	)
	list(APPEND SRCS ${OUTPUT_FILE})
	engine_mark_as_generated(${OUTPUT_FILE})
endmacro()

imgui_build_font(FontAwesomeSolid fa-solid-900.ttf)
imgui_build_font(ForkAwesomeWebFont forkawesome-webfont.ttf)
imgui_build_font(ArimoRegular Arimo-Regular.ttf)

engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES render app dearimgui audio)
target_include_directories(${LIB} PUBLIC . ${GEN_DIR})
