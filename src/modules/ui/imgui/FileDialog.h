/**
 * @file
 * @ingroup UI
 * @note Used https://github.com/Limeoats/L2DFileDialog as a base
 */

#pragma once

#include "video/WindowedApp.h"

namespace ui {
namespace imgui {

/**
 * @param open The visibility state of the dialog. This is set to false if the dialog should get closed.
 * This happens on user input. If the function returns @c true the boolean is set to false to no longer show
 * the dialog.
 * @param buffer Output buffer for the full path of the selected entity
 * @param bufferSize Output buffer size
 * @return @c true if user input was made - either an entity was selected, or the selection was cancelled.
 * @return @c false if no user input was made yet and the dialog should still run
 */
extern bool showFileDialog(bool *open, char *buffer, unsigned int bufferSize, video::WindowedApp::OpenFileMode type);

}
}
