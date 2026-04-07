/**
 * @file
 */

#include "LUAApiWidget.h"
#include "IMGUIEx.h"
#include "LUAScriptParameters.h"
#include "command/CommandHandler.h"
#include "imgui.h"
#include "palette/Palette.h"
#include "voxelgenerator/LUAApi.h"
#include <glm/ext/scalar_constants.hpp>

namespace voxelui {

void LUAApiWidget::clear() {
	_scripts.clear();
	_currentScript = -1;
}

bool LUAApiWidget::updateScriptParameters(voxelgenerator::LUAScript &script, const palette::Palette &palette) {
	renderScriptParameters(script.parameterDescription, script.parameters, &palette);
	return true;
}

bool LUAApiWidget::updateScriptExecutionPanel(voxelgenerator::LUAApi &luaApi, const palette::Palette &palette,
											  LUAApiExecutorContext &ctx, uint32_t flags) {
	if (_scripts.empty()) {
		_scripts = luaApi.listScripts();
	}
	if (_scripts.empty()) {
		return false;
	}
	if (_currentScript == -1) {
		_currentScript = 0;
		if (!_scripts[0].valid) {
			reloadScriptParameters(luaApi, _scripts[0]);
		}
	}
	if (ctx.isRunning) {
		ImGui::Spinner("running_scripts", ImGui::Size(1.0f));
		return true;
	}

	if (ImGui::SearchableComboItems("##script", &_currentScript, _scripts, _scriptSearchFilter)) {
		loadCurrentScript(luaApi);
	}
	ImGui::TooltipTextUnformatted(_("LUA scripts for manipulating the voxel volumes"));

	const voxelgenerator::LUAScript &script = currentScript();
	const bool validScriptIndex = script.valid;
	if (flags & LUAAPI_WIDGET_FLAG_RUN) {
		ImGui::SameLine();

		if (ImGui::DisabledButton(_("Run"), !validScriptIndex)) {
			ctx.runScript(_activeScript, script.parameters);
			core::DynamicArray<core::String> args;
			args.reserve(script.parameters.size() + 1);
			args.push_back(_scripts[_currentScript].filename);
			args.append(script.parameters);
			if (ctx.listener) {
				(*ctx.listener)("xs", script.parameters);
			}
		}
		ImGui::TooltipTextUnformatted(_("Execute the selected script for the currently loaded voxel volumes"));
	}

	ImGui::TextWrappedUnformatted(script.desc.c_str());

	if (voxelgenerator::LUAScript *s = currentScriptPointer()) {
		updateScriptParameters(*s, palette);
	}

	if (flags & LUAAPI_WIDGET_FLAG_NOTIFY) {
		ctx.notify(script.filename, script.parameters);
	}
	return validScriptIndex;
}

void LUAApiWidget::reloadScriptParameters(voxelgenerator::LUAApi &luaApi, voxelgenerator::LUAScript &script) {
	reloadScriptParameters(luaApi, script, luaApi.load(script.filename));
}

void LUAApiWidget::reloadScriptParameters(voxelgenerator::LUAApi &luaApi, voxelgenerator::LUAScript &s, const core::String &luaScript) {
	_activeScript = luaScript;
	if (s.cached) {
		return;
	}
	luaApi.reloadScriptParameters(s, _activeScript);
}

void LUAApiWidget::reloadCurrentScript(voxelgenerator::LUAApi &luaApi) {
	if (voxelgenerator::LUAScript *s = currentScriptPointer()) {
		s->cached = false;
		reloadScriptParameters(luaApi, *s);
	}
}

void LUAApiWidget::loadCurrentScript(voxelgenerator::LUAApi &luaApi) {
	if (voxelgenerator::LUAScript *s = currentScriptPointer()) {
		reloadScriptParameters(luaApi, *s);
	}
}

} // namespace voxelui
