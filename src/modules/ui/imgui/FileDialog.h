/**
 * @file
 * @ingroup UI
 * @note Used https://github.com/Limeoats/L2DFileDialog as a base
 */

#pragma once

namespace ui {
namespace imgui {

enum class FileDialogType { OpenFile, SelectFolder };

extern void showFileDialog(bool *open, char *buffer, unsigned int bufferSize,
						   FileDialogType type = FileDialogType::OpenFile);

}
}
