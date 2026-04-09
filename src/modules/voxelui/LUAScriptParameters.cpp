/**
 * @file
 */

#include "LUAScriptParameters.h"
#include "DragAndDropPayload.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "imgui.h"
#include "palette/Palette.h"
#include "ui/IMGUIEx.h"
#include "voxelgenerator/LUAApi.h"
#include <glm/ext/scalar_constants.hpp>

namespace voxelui {

void renderScriptParameters(const core::DynamicArray<voxelgenerator::LUAParameterDescription> &params,
							core::DynamicArray<core::String> &values,
							const palette::Palette *palette) {
	const int n = (int)params.size();
	if (n > 0 && ImGui::CollapsingHeader(_("Script parameters"), ImGuiTreeNodeFlags_DefaultOpen)) {
		for (int i = 0; i < n; ++i) {
			const voxelgenerator::LUAParameterDescription &p = params[i];
			core::String &str = values[i];
			switch (p.type) {
			case voxelgenerator::LUAParameterType::Integer: {
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
				float val = core::string::toFloat(str);
				if (p.shouldClamp()) {
					const float maxVal = (float)p.maxValue;
					const float minVal = (float)p.minValue;
					const char *format = glm::abs(maxVal - minVal) <= 10.0f ? "%.6f" : "%.3f";
					if (ImGui::DragFloat(p.name.c_str(), &val, 0.005f, minVal, maxVal, format)) {
						str = core::string::toString(val);
					}
				} else if (ImGui::InputFloat(p.name.c_str(), &val)) {
					str = core::string::toString(val);
				}
				break;
			}
			case voxelgenerator::LUAParameterType::String: {
				ImGui::InputText(p.name.c_str(), &str);
				break;
			}
			case voxelgenerator::LUAParameterType::File: {
				ImGui::InputFile(p.name.c_str(), true, &str, nullptr);
				break;
			}
			case voxelgenerator::LUAParameterType::Boolean: {
				bool checked = core::string::toBool(str);
				if (ImGui::Checkbox(p.name.c_str(), &checked)) {
					str = checked ? "1" : "0";
				}
				break;
			}
			case voxelgenerator::LUAParameterType::Enum: {
				core::DynamicArray<core::String> tokens;
				core::string::splitString(p.enumValues, tokens, ",");
				const auto iter = core::find(tokens.begin(), tokens.end(), str);
				int selected = iter == tokens.end() ? 0 : (int)(iter - tokens.begin());
				if (ImGui::ComboItems(p.name.c_str(), &selected, tokens)) {
					str = tokens[selected];
				}
				break;
			}
			case voxelgenerator::LUAParameterType::ColorIndex: {
				int val = core::string::toInt(str);
				if (palette && val >= 0 && val < palette->colorCount()) {
					const float size = ImGui::Height(1);
					const ImVec2 v1 = ImGui::GetCursorScreenPos();
					const ImVec2 v2(v1.x + size, v1.y + size);
					ImDrawList *drawList = ImGui::GetWindowDrawList();
					drawList->AddRectFilled(v1, v2, ImGui::GetColorU32(palette->color(val)));
					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + size);
				}
				if (ImGui::InputInt(p.name.c_str(), &val)) {
					if (!palette || (val >= 0 && val < palette->colorCount())) {
						str = core::string::toString(val);
					}
				}
				if (palette && ImGui::BeginDragDropTarget()) {
					if (const ImGuiPayload *payload =
							ImGui::AcceptDragDropPayload(voxelui::dragdrop::PaletteIndexPayload)) {
						const int palIdx = *(const uint8_t *)payload->Data;
						str = core::string::toString(palIdx);
					}
					ImGui::EndDragDropTarget();
				}
				break;
			}
			case voxelgenerator::LUAParameterType::HexColor: {
				uint8_t r = 255, g = 0, b = 255, a = 255;
				core::string::parseHex(str.c_str(), r, g, b, a);
				ImVec4 col(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
				if (ImGui::ColorEdit4(p.name.c_str(), &col.x,
									  ImGuiColorEditFlags_Uint8 | ImGuiColorEditFlags_DisplayHex |
										  ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_AlphaBar)) {
					r = (uint8_t)(col.x * 255.0f + 0.5f);
					g = (uint8_t)(col.y * 255.0f + 0.5f);
					b = (uint8_t)(col.z * 255.0f + 0.5f);
					a = (uint8_t)(col.w * 255.0f + 0.5f);
					if (a == 255) {
						str = core::String::format("#%02X%02X%02X", r, g, b);
					} else {
						str = core::String::format("#%02X%02X%02X%02X", r, g, b, a);
					}
				}
				break;
			}
			default:
				break;
			}
			if (!p.description.empty()) {
				ImGui::TooltipTextUnformatted(p.description.c_str());
			}
		}
	}
}

} // namespace voxelui
