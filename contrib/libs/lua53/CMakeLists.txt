set(SRCS
	lapi.c
	lauxlib.c
	lbaselib.c
	lbitlib.c
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

set(LIB lua53)
engine_add_library(
	LIB ${LIB}
	SRCS ${SRCS}
	UNITY
	DEFINES LUA_USE_LONGJMP LUA_COMPAT_MODULE
)
# TODO: compile lua and luac binaries to check if there is no system wide installation
# see common.cmake
if (NOT LUA53_FOUND)
	target_link_libraries(${LIB} ${CMAKE_DL_LIBS})
	if (LINUX AND NOT ANDROID AND NOT EMSCRIPTEN)
		target_compile_definitions(${LIB} PUBLIC -DLUA_USE_LINUX)
	elseif(DARWIN)
		target_compile_definitions(${LIB} PUBLIC -DLUA_USE_MACOSX)
	endif()
	target_compile_definitions(${LIB} PRIVATE "-DLUA_USE_APICHECK")
	if (ANDROID OR EMSCRIPTEN)
		target_compile_definitions(${LIB} PUBLIC -D\"lua_getlocaledecpoint\(\)='.'\")
		target_compile_definitions(${LIB} PUBLIC -DLUA_ANSI -DLUA_USE_C89)
	endif()
	engine_add_executable(TARGET luac SRCS luac.c NOINSTALL)
	target_link_libraries(luac ${LIB})
endif()
