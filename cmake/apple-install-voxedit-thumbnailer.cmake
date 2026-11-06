# Embed the Quick Look thumbnail extension and helper into an installed voxedit.app,
# then ad-hoc codesign the appex and host bundle.
#
# Required -D defines:
#   VOXEDIT_BUNDLE   - absolute path to the installed vengi-voxedit.app
#   APPEX_SOURCE     - absolute path to the built voxelthumb bundle directory
#   APPEX_NAME       - destination folder name (e.g. vengi-voxelthumb.appex)
#   THUMBNAILER_BIN  - absolute path to the vengi-thumbnailer executable
# Optional:
#   HOST_ENTITLEMENTS
#   APPEX_ENTITLEMENTS

if (NOT VOXEDIT_BUNDLE OR NOT APPEX_SOURCE OR NOT APPEX_NAME OR NOT THUMBNAILER_BIN)
	message(FATAL_ERROR "VOXEDIT_BUNDLE, APPEX_SOURCE, APPEX_NAME and THUMBNAILER_BIN are required")
endif()

if (NOT EXISTS "${VOXEDIT_BUNDLE}")
	message(FATAL_ERROR "Installed voxedit bundle not found: ${VOXEDIT_BUNDLE}")
endif()
if (NOT EXISTS "${APPEX_SOURCE}")
	message(FATAL_ERROR "Quick Look extension bundle not found: ${APPEX_SOURCE}")
endif()
if (NOT EXISTS "${THUMBNAILER_BIN}")
	message(FATAL_ERROR "Thumbnailer helper not found: ${THUMBNAILER_BIN}")
endif()

file(MAKE_DIRECTORY "${VOXEDIT_BUNDLE}/Contents/PlugIns")
file(MAKE_DIRECTORY "${VOXEDIT_BUNDLE}/Contents/MacOS")

set(_appex_dst "${VOXEDIT_BUNDLE}/Contents/PlugIns/${APPEX_NAME}")
file(REMOVE_RECURSE "${_appex_dst}")
file(COPY "${APPEX_SOURCE}/" DESTINATION "${_appex_dst}")

get_filename_component(_helper_name "${THUMBNAILER_BIN}" NAME)
file(COPY "${THUMBNAILER_BIN}" DESTINATION "${VOXEDIT_BUNDLE}/Contents/MacOS"
	FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

find_program(_CODESIGN codesign)
if (_CODESIGN)
	if (APPEX_ENTITLEMENTS AND EXISTS "${APPEX_ENTITLEMENTS}")
		execute_process(
			COMMAND ${_CODESIGN} --force --sign - --entitlements "${APPEX_ENTITLEMENTS}" "${_appex_dst}"
			RESULT_VARIABLE _sign_appex
		)
		if (NOT _sign_appex EQUAL 0)
			message(WARNING "codesign failed for ${_appex_dst} (result ${_sign_appex})")
		endif()
	endif()
	if (HOST_ENTITLEMENTS AND EXISTS "${HOST_ENTITLEMENTS}")
		execute_process(
			COMMAND ${_CODESIGN} --force --deep --sign - --entitlements "${HOST_ENTITLEMENTS}" "${VOXEDIT_BUNDLE}"
			RESULT_VARIABLE _sign_app
		)
		if (NOT _sign_app EQUAL 0)
			message(WARNING "codesign failed for ${VOXEDIT_BUNDLE} (result ${_sign_app})")
		endif()
	endif()
else()
	message(STATUS "codesign not found; skipped signing of embedded Quick Look pieces")
endif()
