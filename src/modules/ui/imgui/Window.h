/**
 * @file
 */

#pragma once

#include "core/Enum.h"
#include "core/String.h"

namespace ui {
namespace imgui {

enum class WindowFlag {
	None = 0,
	FullScreen = 1 << 0,
	Minimized = 1 << 1,
	NoTitle = 1 << 2,
	FixedPosition = 1 << 3,
	Modal = 1 << 4,
	NoBackground = 1 << 5,
	Centered = 1 << 6,
	FixedSize = 1 << 7,
	NoCollapse = 1 << 8
};
CORE_ENUM_BIT_OPERATIONS(WindowFlag)

/**
 * @brief A Window that is managed by the @c WindowStack class
 */
class Window {
protected:
	WindowFlag _flags = WindowFlag::None;
	core::String _title;
	core::String _music;

	void close();
	void open(const core::String &name);
	virtual void render(double deltaFrameSeconds) = 0;

public:
	Window(const core::String &title) : _title(title) {
	}

	virtual ~Window() {
	}

	const core::String &backgroundMusic() const {
		return _music;
	}

	void setBackgroundMusic(const core::String &music) {
		_music = music;
	}

	inline void addFlags(WindowFlag flags) {
		_flags |= flags;
	}

	inline void setFlags(WindowFlag flags) {
		_flags = flags;
	}

	inline void setCentered() {
		_flags |= WindowFlag::Centered;
	}

	inline void setNoCollapse() {
		_flags |= WindowFlag::NoCollapse;
	}

	inline void setFixedPosition() {
		_flags |= WindowFlag::FixedPosition;
	}

	inline void setFixedSize() {
		_flags |= WindowFlag::FixedSize;
	}

	inline void setNoBackground() {
		_flags |= WindowFlag::NoBackground;
	}

	inline void setFullscreen() {
		_flags |= WindowFlag::FullScreen;
	}

	inline bool isFullscreen() const {
		return checkFlags(WindowFlag::FullScreen);
	}

	inline void setMinimize() {
		_flags |= WindowFlag::Minimized;
	}

	inline bool isMinimized() const {
		return checkFlags(WindowFlag::Minimized);
	}

	inline bool checkFlags(WindowFlag flags) const {
		return (_flags & flags) == flags;
	}

	virtual void update(double deltaFrameSeconds, bool top);
};

} // namespace imgui
} // namespace ui
