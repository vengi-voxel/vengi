/**
 * @file
 */

#pragma once

#include "core/Var.h"
#include "imgui.h"

namespace ui {

/**
 * @brief Helper for dockable panels whose open state is bound to a bool cvar.
 *
 * Intended to be declared @c static so @c getVar runs once:
 * @code
 * static ui::ScopedPanel panel(cfg::VoxEditShowPalette);
 * if (!panel.isOpen()) {
 *   return;
 * }
 * if (ui::ScopedPanel::Scope scope = panel.begin(title, flags)) {
 *   // content
 * }
 * @endcode
 *
 * @c Scope calls ImGui::End in its destructor and writes false back to the cvar if the
 * window was closed.
 */
class ScopedPanel {
private:
	core::VarPtr _var;

public:
	explicit ScopedPanel(const char *varName) : _var(core::getVar(varName)) {
	}

	ScopedPanel(const ScopedPanel &) = delete;
	ScopedPanel &operator=(const ScopedPanel &) = delete;

	bool isOpen() const {
		return _var->boolVal();
	}

	class Scope {
	private:
		core::VarPtr _var;
		bool _open = false;
		bool _began = false;
		bool _visible = false;

	public:
		Scope() = default;
		Scope(const core::VarPtr &var, const char *name, ImGuiWindowFlags flags) : _var(var) {
			_open = var->boolVal();
			if (!_open) {
				return;
			}
			_visible = ImGui::Begin(name, &_open, flags);
			_began = true;
		}
		Scope(Scope &&other) noexcept
			: _var(core::move(other._var)), _open(other._open), _began(other._began), _visible(other._visible) {
			other._began = false;
		}
		Scope(const Scope &) = delete;
		Scope &operator=(const Scope &) = delete;
		Scope &operator=(Scope &&) = delete;

		~Scope() {
			if (_began) {
				ImGui::End();
				if (!_open) {
					_var->setVal(false);
				}
			}
		}

		explicit operator bool() const {
			return _visible;
		}
	};

	Scope begin(const char *name, ImGuiWindowFlags flags = 0) {
		return Scope(_var, name, flags);
	}
};

} // namespace ui
