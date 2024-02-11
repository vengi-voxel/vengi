enable_language(OBJC)
enable_language(OBJCXX)

set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++14")
set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
set(CMAKE_XCODE_ATTRIBUTE_GCC_ENABLE_CPP_EXCEPTIONS "YES")
set(CMAKE_XCODE_ATTRIBUTE_GCC_ENABLE_CPP_RTTI "YES")
set(CMAKE_XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC "NO")
set(CMAKE_XCODE_GENERATE_SCHEME "YES")

if(APPLE)
	find_program(XCRUN_EXECUTABLE NAMES xcrun)
	if (XCRUN_EXECUTABLE)
		# execute xrun to find the development team id
		execute_process(COMMAND ${XCRUN_EXECUTABLE} security find-identity -v -p codesigning OUTPUT_VARIABLE DEVELOPMENT_TEAMS)
		# extract the team id from the output that starts with "Developer ID Application: "
		string(REGEX MATCH "Developer ID Application: [^)]+\\(([^)]+)\\)" DEVELOPMENT_TEAM_ID ${DEVELOPMENT_TEAMS})
		set(DEVELOPMENT_TEAM_ID ${CMAKE_MATCH_1})
		if (DEVELOPMENT_TEAM_ID)
			message(STATUS "Development Team ID: ${DEVELOPMENT_TEAM_ID}")
		else()
			message(WARNING "No Development Team ID found")
		endif()
	endif()

	# With xcode versions >= 14 it is no longer possible to build unsigned binaries
	# - is for signing locally
	if (DEVELOPMENT_TEAM_ID STREQUAL "")
		if(XCODE_VERSION VERSION_LESS 14)
			set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "" CACHE INTERNAL "")
		else()
			set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "-" CACHE INTERNAL "")
		endif()
	else()
		set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Mac Developer" CACHE INTERNAL "")
		set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ${DEVELOPMENT_TEAM_ID})
	endif()
	# --deep is for signing 3d party libraries
	set(CMAKE_XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "--deep -o linker-signed --timestamp" CACHE INTERNAL "")
endif()
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstrict-aliasing -Wno-multichar -Wall -Wextra -Wno-unused-parameter -Wno-unknown-pragmas -Wno-ignored-qualifiers -Wno-long-long -Wno-overloaded-virtual -Wno-unused-volatile-lvalue -Wno-deprecated-writable-strings -Wno-unknown-warning-option")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -DDEBUG -g")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -Wnon-virtual-dtor")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${CMAKE_C_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${CMAKE_C_FLAGS_DEBUG}")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ObjC -dead_strip -lpthread")
if (USE_SANITIZER)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -fsanitize=undefined")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=undefined")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address -fsanitize=undefined")
endif()
