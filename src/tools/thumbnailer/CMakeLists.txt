project(thumbnailer)
set(SRCS
	Thumbnailer.h Thumbnailer.cpp
	Shared.h Shared.cpp
)

engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} DESCRIPTION "Command line voxel thumbnailer")
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES app voxelrender voxelpathtracer voxelformat)

set(RUNNER_SRCS
	ThumbnailerRunner.h ThumbnailerRunner.cpp
)

if (UNIX)
	enable_testing()
	configure_file(tests/tests.sh.in ${PROJECT_NAME}-tests.sh @ONLY)
	add_test(NAME shelltests-${PROJECT_NAME} COMMAND ${PROJECT_NAME}-tests.sh $<TARGET_FILE:${PROJECT_NAME}>)
	set_tests_properties(shelltests-${PROJECT_NAME} PROPERTIES DEPENDS ${PROJECT_NAME})
endif()

if (UNIX)
	set(TEST_SRCS
		${RUNNER_SRCS}
		tests/ThumbnailerRunnerTest.cpp
	)
	gtest_suite_begin(tests-${PROJECT_NAME} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
	gtest_suite_sources(tests-${PROJECT_NAME} ${TEST_SRCS})
	gtest_suite_deps(tests-${PROJECT_NAME} core io)
	gtest_suite_end(tests-${PROJECT_NAME})
	if (TARGET tests-${PROJECT_NAME})
		target_include_directories(tests-${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
	endif()
endif()

if (WIN32)
	set(WIN_SRCS
		windows/ThumbnailerDLL.cpp
		windows/ThumbnailerDLL.def
		windows/ThumbnailerProvider.cpp windows/ThumbnailerProvider.h
	)
	engine_add_sharedlibrary(TARGET voxelthumb SRCS ${WIN_SRCS})
	target_compile_definitions(voxelthumb PRIVATE -DWINDOWS_THUMBNAILER_DLL)
	set_target_properties(voxelthumb PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS OFF)
	target_link_libraries(voxelthumb voxelformat shlwapi windowscodecs ole32)
	add_dependencies(voxelthumb ${PROJECT_NAME})
	set(HAS_ICON 0)
	set(NAME "Vengi thumbnailer")
	set(DESCRIPTION "Voxel thumbnailer for Windows Explorer")
	set(ORIGINAL_FILENAME "${CMAKE_PROJECT_NAME}-voxelthumb.dll")
	string(TIMESTAMP COPYRIGHT_YEAR "%Y")
	string(TIMESTAMP COPYRIGHT_MONTH "%B")
	configure_file(${ROOT_DIR}/contrib/installer/windows/application.rc.in ${CMAKE_CURRENT_BINARY_DIR}/voxelthumb.rc @ONLY)
	target_sources(voxelthumb PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/voxelthumb.rc)
	cpack_add_component(voxelthumb DISPLAY_NAME "Windows Explorer thumbnails" DESCRIPTION "Voxel thumbnails in Windows Explorer" DEPENDS thumbnailer)
endif()

if (APPLE)
	enable_language(OBJCXX)
	set(MAC_SRCS
		${RUNNER_SRCS}
		macos/ThumbnailProvider.mm
	)
	# Use a distinct target/plist name from the CLI "thumbnailer" app so
	# contrib/installer/osx/voxelthumb.plist.in is not picked up by the CLI.
	add_executable(voxelthumb MACOSX_BUNDLE ${MAC_SRCS})
	target_include_directories(voxelthumb PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
	target_compile_definitions(voxelthumb PRIVATE SDL_MAIN_HANDLED)
	set_source_files_properties(macos/ThumbnailProvider.mm PROPERTIES COMPILE_FLAGS "-fobjc-arc")
	set_target_properties(voxelthumb PROPERTIES
		OUTPUT_NAME "${CMAKE_PROJECT_NAME}-voxelthumb"
		MACOSX_BUNDLE TRUE
		BUNDLE_EXTENSION appex
		XCODE_PRODUCT_TYPE "com.apple.product-type.app-extension"
		XCODE_ATTRIBUTE_WRAPPER_EXTENSION appex
		XCODE_ATTRIBUTE_GENERATE_PKGINFO_FILE YES
		XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${ROOT_DIR}/contrib/installer/osx/thumbnailer.entitlements"
		MACOSX_BUNDLE_INFO_PLIST "${ROOT_DIR}/contrib/installer/osx/voxelthumb.plist.in"
		MACOSX_BUNDLE_GUI_IDENTIFIER "vengi.voxedit.thumbnail"
		MACOSX_BUNDLE_BUNDLE_NAME "Vengi Thumbnailer"
		MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}"
		MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
		ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/voxelthumb"
		LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/voxelthumb"
		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/voxelthumb"
	)
	target_link_libraries(voxelthumb PRIVATE core io
		"-framework QuickLookThumbnailing"
		"-framework Foundation"
		"-framework AppKit"
		"-framework UniformTypeIdentifiers"
	)
	# App extensions use NSExtensionMain instead of main().
	target_link_options(voxelthumb PRIVATE "LINKER:-e,_NSExtensionMain")
	add_dependencies(voxelthumb ${PROJECT_NAME})

	add_custom_command(TARGET voxelthumb POST_BUILD
		COMMAND codesign --force --sign - --entitlements "${ROOT_DIR}/contrib/installer/osx/thumbnailer.entitlements" "$<TARGET_BUNDLE_DIR:voxelthumb>"
		COMMENT "Ad-hoc sign Quick Look thumbnail extension"
		VERBATIM
	)

	set(_VOXEDIT_APP "${CMAKE_PROJECT_NAME}-voxedit.app")
	set(_VOXTHUMB_APPEX "${CMAKE_PROJECT_NAME}-voxelthumb.appex")

	# Embed into the build-tree voxedit bundle for local runs without install/cpack.
	if (TARGET voxedit)
		add_custom_target(embed-voxedit-qlthumbnailer ALL
			COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:voxedit>/Contents/PlugIns"
			COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:voxedit>/Contents/MacOS"
			COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_BUNDLE_DIR:voxedit>/Contents/PlugIns/${_VOXTHUMB_APPEX}"
			COMMAND ${CMAKE_COMMAND} -E copy_directory
				"$<TARGET_BUNDLE_DIR:voxelthumb>"
				"$<TARGET_BUNDLE_DIR:voxedit>/Contents/PlugIns/${_VOXTHUMB_APPEX}"
			COMMAND ${CMAKE_COMMAND} -E copy
				"$<TARGET_FILE:${PROJECT_NAME}>"
				"$<TARGET_BUNDLE_DIR:voxedit>/Contents/MacOS/"
			DEPENDS voxedit voxelthumb ${PROJECT_NAME}
			COMMENT "Embed Quick Look thumbnail extension and helper into voxedit (build tree)"
			VERBATIM
		)
	endif()

	# Install into the packaged/installed voxedit.app so CPack DragNDrop ships Finder integration
	# inside the editor bundle (not as a separate top-level appex in the DMG root).
	install(CODE "
		execute_process(
			COMMAND \"${CMAKE_COMMAND}\"
				-DVOXEDIT_BUNDLE=\${CMAKE_INSTALL_PREFIX}/${_VOXEDIT_APP}
				-DAPPEX_SOURCE=$<TARGET_BUNDLE_DIR:voxelthumb>
				-DAPPEX_NAME=${_VOXTHUMB_APPEX}
				-DTHUMBNAILER_BIN=$<TARGET_FILE:${PROJECT_NAME}>
				-DHOST_ENTITLEMENTS=${ROOT_DIR}/contrib/installer/osx/entitlements.plist
				-DAPPEX_ENTITLEMENTS=${ROOT_DIR}/contrib/installer/osx/thumbnailer.entitlements
				-P \"${ROOT_DIR}/cmake/apple-install-voxedit-thumbnailer.cmake\"
			RESULT_VARIABLE _embed_result
		)
		if (NOT _embed_result EQUAL 0)
			message(FATAL_ERROR \"Failed to embed Quick Look thumbnailer into ${_VOXEDIT_APP}\")
		endif()
	" COMPONENT voxedit)
endif()
