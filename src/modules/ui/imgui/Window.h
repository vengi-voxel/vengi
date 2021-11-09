/**
 * @file
 */

#pragma once

#include "core/Enum.h"
#include "core/String.h"

namespace ui {
namespace imgui {

enum class WindowFlag { None = 0, FullScreen = 1 << 0, Minimized = 1 << 1 };
CORE_ENUM_BIT_OPERATIONS(WindowFlag)

class Window {
protected:
	WindowFlag _flags = WindowFlag::None;
	core::String _title;

	void close();
	void open(const core::String &name);
	virtual void render() = 0;

public:
	virtual ~Window() {
	}

	inline void toggleFullscreen() {
		_flags |= WindowFlag::FullScreen;
	}

	inline bool isFullscreen() const {
		return checkFlags(WindowFlag::FullScreen);
	}

	inline void toggleMinimize() {
		_flags |= WindowFlag::Minimized;
	}

	inline bool isMinimized() const {
		return checkFlags(WindowFlag::Minimized);
	}

	inline bool checkFlags(WindowFlag flags) const {
		return (_flags & flags) == flags;
	}

	virtual void update();
};

} // namespace imgui
} // namespace ui
