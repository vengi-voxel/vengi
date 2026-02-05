/**
 * @file
 */

#include "LUAApiWidget.h"
#include "DragAndDropPayload.h"
#include "IMGUIEx.h"
#include "command/CommandHandler.h"
#include "core/StringUtil.h"
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
	const int n = (int)script.parameterDescription.size();
	if (n && ImGui::CollapsingHeader(_("Script parameters"), ImGuiTreeNodeFlags_DefaultOpen)) {
		for (int i = 0; i < n; ++i) {
			const voxelgenerator::LUAParameterDescription &p = script.parameterDescription[i];
			switch (p.type) {
			case voxelgenerator::LUAParameterType::ColorIndex: {
				core::String &str = script.parameters[i];
				int val = core::string::toInt(str);
				if (val >= 0 && val < palette.colorCount()) {
					const float size = ImGui::Height(1);
					const ImVec2 v1 = ImGui::GetCursorScreenPos();
					const ImVec2 v2(v1.x + size, v1.y + size);
					ImDrawList *drawList = ImGui::GetWindowDrawList();
					drawList->AddRectFilled(v1, v2, ImGui::GetColorU32(palette.color(val)));
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + size);
				}
				if (ImGui::InputInt(p.name.c_str(), &val)) {
					if (val >= 0 && val < palette.colorCount()) {
						str = core::string::toString(val);
					}
				}

				if (ImGui::BeginDragDropTarget()) {
					if (const ImGuiPayload *payload =
							ImGui::AcceptDragDropPayload(voxelui::dragdrop::PaletteIndexPayload)) {
						const int palIdx = *(const uint8_t *)payload->Data;
						str = core::string::toString(palIdx);
					}
					ImGui::EndDragDropTarget();
				}

				break;
			}
			case voxelgenerator::LUAParameterType::Integer: {
				core::String &str = script.parameters[i];
				int val = core::string::toInt(str);
				if (p.shouldClamp()) {
					int maxVal = (int)(p.maxValue + glm::epsilon<double>());
					int minVal = (int)(p.minValue + glm::epsilon<double>());
					if (ImGui::DragInt(p.name.c_str(), &val, 1.0f, minVal, maxVal)) {
						str = core::string::toString(val);
					}
				} else if (ImGui::InputInt(p.name.c_str(), &val)) {
					str = core::string::toString(val);
				}
				break;
			}
			case voxelgenerator::LUAParameterType::Float: {
				core::String &str = script.parameters[i];
				float val = core::string::toFloat(str);
				if (p.shouldClamp()) {
					const float maxVal = (float)p.maxValue;
					const float minVal = (float)p.minValue;
					const char *format;
					if (glm::abs(maxVal - minVal) <= 10.0f) {
						format = "%.6f";
					} else {
						format = "%.3f";
					}

					if (ImGui::DragFloat(p.name.c_str(), &val, 0.005f, minVal, maxVal, format)) {
						str = core::string::toString(val);
					}
				} else if (ImGui::InputFloat(p.name.c_str(), &val)) {
					str = core::string::toString(val);
				}
				break;
			}
			case voxelgenerator::LUAParameterType::String: {
				core::String &str = script.parameters[i];
				ImGui::InputText(p.name.c_str(), &str);
				break;
			}
			case voxelgenerator::LUAParameterType::File: {
				core::String &str = script.parameters[i];
				ImGui::InputFile(p.name.c_str(), true, &str, nullptr);
				break;
			}
			case voxelgenerator::LUAParameterType::Enum: {
				core::String &str = script.parameters[i];
				core::DynamicArray<core::String> tokens;
				core::string::splitString(script.enumValues[i], tokens, ",");
				const auto iter = core::find(tokens.begin(), tokens.end(), str);
				int selected = iter == tokens.end() ? 0 : iter - tokens.begin();
				if (ImGui::ComboItems(p.name.c_str(), &selected, tokens)) {
					str = tokens[selected];
				}
				break;
			}
			case voxelgenerator::LUAParameterType::Boolean: {
				core::String &str = script.parameters[i];
				bool checked = core::string::toBool(str);
				if (ImGui::Checkbox(p.name.c_str(), &checked)) {
					str = checked ? "1" : "0";
				}
				break;
			}
			case voxelgenerator::LUAParameterType::Max:
				return script.valid;
			}
			if (!p.description.empty()) {
				ImGui::TooltipTextUnformatted(p.description.c_str());
			}
		}
	}
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

	if (ImGui::ComboItems("##script", &_currentScript, _scripts)) {
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
