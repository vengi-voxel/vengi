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

engine_add_library(
	LIB lua53
	SRCS ${SRCS}
	DEFINES LUA_USE_LONGJMP LUA_COMPAT_MODULE
)
if (NOT LUA53_FOUND)
	if (LINUX AND NOT ANDROID AND NOT EMSCRIPTEN)
		target_compile_definitions(lua53 PUBLIC -DLUA_USE_LINUX)
	elseif(DARWIN)
		target_compile_definitions(lua53 PUBLIC -DLUA_USE_MACOSX)
	endif()
	target_compile_definitions(lua53 PRIVATE "-DLUA_USE_APICHECK")
	if (ANDROID OR EMSCRIPTEN)
		target_compile_definitions(lua53 PUBLIC -D\"lua_getlocaledecpoint\(\)='.'\")
		target_compile_definitions(lua53 PUBLIC -DLUA_ANSI -DLUA_USE_C89)
	endif()
endif()
