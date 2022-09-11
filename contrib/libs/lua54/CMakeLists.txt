set(SRCS
	lapi.c
	lauxlib.c
	lbaselib.c
	lcode.c
	lcorolib.c
	lctype.c
	ldblib.c
	ldebug.c
	ldo.c
	ldump.c
	lfunc.c
	lgc.c
	linit.c
	liolib.c
	llex.c
	lmathlib.c
	lmem.c
	loadlib.c
	lobject.c
	lopcodes.c
	loslib.c
	lparser.c
	lstate.c
	lstring.c
	lstrlib.c
	ltable.c
	ltablib.c
	ltm.c
	lundump.c
	lutf8lib.c
	lvm.c
	lzio.c
	)

set(LIB lua54)
engine_add_library(
	LIB ${LIB}
	SRCS ${SRCS}
)

function(configure_lua TARGET)
	target_link_libraries(${TARGET} ${CMAKE_DL_LIBS})
	target_compile_definitions(${TARGET} PUBLIC LUA_USE_LONGJMP LUA_COMPAT_MODULE)
	if (LINUX AND NOT ANDROID AND NOT EMSCRIPTEN)
		target_compile_definitions(${TARGET} PUBLIC -DLUA_USE_LINUX)
	elseif(DARWIN)
		target_compile_definitions(${TARGET} PUBLIC -DLUA_USE_MACOSX)
	endif()
	target_compile_definitions(${TARGET} PRIVATE "-DLUA_USE_APICHECK")
	if (ANDROID OR EMSCRIPTEN)
		target_compile_definitions(${TARGET} PUBLIC -D\"lua_getlocaledecpoint\(\)='.'\")
		target_compile_definitions(${TARGET} PUBLIC -DLUA_ANSI -DLUA_USE_C89)
	endif()
endfunction()

if (NOT LUA54_FOUND)
	configure_lua(${LIB})
endif()

find_program(LUAC_EXECUTABLE NAMES luac luac5.4)
if (LUAC_EXECUTABLE)
	execute_process(
		COMMAND "${LUAC_EXECUTABLE}" -v
		OUTPUT_VARIABLE LUAC_VERSION_LINE
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	separate_arguments(LUAC_VERSION_LINE)
	if (LUAC_VERSION_LINE)
		list(GET LUAC_VERSION_LINE 1 LUAC_VERSION)
		if (LUAC_VERSION VERSION_LESS "5.4")
			unset(LUAC_EXECUTABLE)
			message(STATUS "luac version ${LUAC_VERSION} found - ignore this one")
		else()
			message(STATUS "luac version ${LUAC_VERSION} found")
		endif()
	else()
		unset(LUAC_EXECUTABLE)
		message(STATUS "Failed to parse luac version ${LUAC_VERSION_LINE}")
	endif()
endif()

if (NOT LUAC_EXECUTABLE)
	message(STATUS "luac not found - use bundled version")
	add_executable(luac luac.c ${SRCS})
	configure_lua(luac)
else()
	message(STATUS "${LUAC_EXECUTABLE} found")
	add_executable(luac IMPORTED GLOBAL)
	set_property(TARGET luac PROPERTY IMPORTED_LOCATION ${LUAC_EXECUTABLE})
endif()
