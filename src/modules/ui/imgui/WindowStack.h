/**
 * @file
 */

#pragma once

#include "Window.h"
#include "core/IComponent.h"
#include "core/collection/Stack.h"
#include "core/collection/StringMap.h"
namespace ui {
namespace imgui {

class WindowStack : public core::IComponent {
private:
	core::StringMap<Window *> _windows;
	core::Stack<Window *, 128> _stack;

public:
	bool init();
	void shutdown();
	void construct();

	void render();

	/**
	 * @note The window stack is taking ownership of the given window
	 */
	bool registerWindow(const core::String &name, Window *window);

	bool setNewRootWindow(const core::String &name);
	bool push(const core::String &name);
	bool pop();
};

} // namespace imgui
} // namespace ui
