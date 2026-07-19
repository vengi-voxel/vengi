# Install a .desktop file and app icon into the current user's XDG data dirs.
# Used for from-source builds on Wayland, where SDL_SetWindowIcon is often unsupported
# and the compositor resolves icons via the desktop file + icon theme.
#
# Required -D defines: BINARY, DESKTOP_SRC, APP_ID, APPICON
# Optional: ICON_PNG

if (NOT DEFINED ENV{HOME} OR "$ENV{HOME}" STREQUAL "")
	message(FATAL_ERROR "HOME is not set")
endif()
if (NOT BINARY OR NOT DESKTOP_SRC OR NOT APP_ID OR NOT APPICON)
	message(FATAL_ERROR "BINARY, DESKTOP_SRC, APP_ID and APPICON are required")
endif()
if (NOT EXISTS "${DESKTOP_SRC}")
	message(FATAL_ERROR "Desktop file not found: ${DESKTOP_SRC}")
endif()
if (NOT EXISTS "${BINARY}")
	message(FATAL_ERROR "Binary not found: ${BINARY}")
endif()

set(_app_dir "$ENV{HOME}/.local/share/applications")
set(_icon_dir "$ENV{HOME}/.local/share/icons/hicolor/128x128/apps")
file(MAKE_DIRECTORY "${_app_dir}")
file(MAKE_DIRECTORY "${_icon_dir}")

file(READ "${DESKTOP_SRC}" _desktop)
string(REGEX REPLACE "\r\n" "\n" _desktop "${_desktop}")
string(REGEX REPLACE "Exec=[^\n]*" "Exec=${BINARY} %f" _desktop "${_desktop}")

set(_icon_installed FALSE)
if (ICON_PNG AND EXISTS "${ICON_PNG}")
	file(COPY "${ICON_PNG}" DESTINATION "${_icon_dir}")
	set(_icon_installed TRUE)
	# Absolute Icon= works even before the icon theme cache is refreshed
	string(REGEX REPLACE "Icon=[^\n]*" "Icon=${_icon_dir}/${APPICON}.png" _desktop "${_desktop}")
endif()

set(_desktop_dst "${_app_dir}/${APP_ID}.desktop")
file(WRITE "${_desktop_dst}" "${_desktop}")

execute_process(COMMAND update-desktop-database "${_app_dir}" ERROR_QUIET)
execute_process(COMMAND gtk-update-icon-cache -f -t "$ENV{HOME}/.local/share/icons/hicolor" ERROR_QUIET)

message(STATUS "Installed ${_desktop_dst}")
if (_icon_installed)
	message(STATUS "Installed ${_icon_dir}/${APPICON}.png")
endif()
message(STATUS "Restart the app (or log out/in) if the dock icon does not update yet")
