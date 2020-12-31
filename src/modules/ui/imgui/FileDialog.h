/**
 * @file
 * @ingroup UI
 * @note Used https://github.com/Limeoats/L2DFileDialog as a base
 */

#pragma once

#include "video/WindowedApp.h"

namespace ui {
namespace imgui {

extern void showFileDialog(bool *open, char *buffer, unsigned int bufferSize, video::WindowedApp::OpenFileMode type);

}
}
