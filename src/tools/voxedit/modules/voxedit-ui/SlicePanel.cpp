/**
 * @file
 */

#include "SlicePanel.h"
#include "app/I18N.h"
#include "core/Algorithm.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "ui/ScopedPanel.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace voxedit {

// Back: +Z looking towards -Z. Right: +X towards -X. Top: +Y towards -Y.
static constexpr int DepthAxis[] = {2, 0, 1};
static constexpr int HorizontalAxis[] = {0, 2, 0};
static constexpr int VerticalAxis[] = {1, 1, 2};

SlicePanel::~SlicePanel() {
	finishStroke();
}

glm::ivec3 SlicePanel::voxelPosition(int view, const voxel::Region &region, int slice, int u, int v) {
	glm::ivec3 pos = region.getLowerCorner();
	pos[DepthAxis[view]] = slice;
	pos[HorizontalAxis[view]] = view == 1 ? region.getUpperCorner().z - u : pos[HorizontalAxis[view]] + u;
	pos[VerticalAxis[view]] = view == 2 ? pos.z + v : region.getUpperCorner().y - v;
	return pos;
}

glm::ivec2 SlicePanel::viewPosition(int view, const voxel::Region &region, const glm::ivec3 &pos) {
	const int u = view == 1 ? region.getUpperCorner().z - pos.z : pos.x - region.getLowerCorner().x;
	const int v = view == 2 ? pos.z - region.getLowerCorner().z : region.getUpperCorner().y - pos.y;
	return glm::ivec2(u, v);
}

void SlicePanel::drawTarget(const voxel::Region &region) {
	for (int view = 0; view < 3; ++view) {
		const TargetView &targetView = _targetViews[view];
		ImDrawList *draw = targetView.draw;
		if (draw == nullptr) {
			continue;
		}
		const glm::ivec2 cellPos = viewPosition(view, region, _target);
		const ImVec2 center(targetView.origin.x + (cellPos.x + 0.5f) * targetView.cell,
							targetView.origin.y + (cellPos.y + 0.5f) * targetView.cell);
		draw->PushClipRect(targetView.min, targetView.max, false);
		// A dark outline keeps the target visible over both light and dark palette colors.
		for (int pass = 0; pass < 2; ++pass) {
			const ImU32 color = pass == 0 ? IM_COL32(0, 0, 0, 230) : IM_COL32(255, 235, 100, 255);
			const float thickness = pass == 0 ? 3.0f : 1.0f;
			const float gap = targetView.cell * 0.5f;
			draw->AddLine(ImVec2(targetView.min.x, center.y), ImVec2(center.x - gap, center.y), color, thickness);
			draw->AddLine(ImVec2(center.x + gap, center.y), ImVec2(targetView.max.x, center.y), color, thickness);
			draw->AddLine(ImVec2(center.x, targetView.min.y), ImVec2(center.x, center.y - gap), color, thickness);
			draw->AddLine(ImVec2(center.x, center.y + gap), ImVec2(center.x, targetView.max.y), color, thickness);
			draw->AddRect(ImVec2(center.x - gap, center.y - gap), ImVec2(center.x + gap, center.y + gap), color, 0.0f,
						  thickness);
		}
		draw->PopClipRect();
	}
}

void SlicePanel::finishStroke() {
	if (_stroke) {
		if (_strokeRegion.isValid()) {
			if (const scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNodeByUUID(_strokeNodeUUID)) {
				_sceneMgr->mementoHandler().markModification(_sceneMgr->sceneGraph(), *node, _strokeRegion);
			}
		}
		_stroke = false;
	}
	_strokeRegion = voxel::Region::InvalidRegion;
	_lastPosValid = false;
	_strokeView = -1;
}

void SlicePanel::editLine(scenegraph::SceneGraphNode &node, const glm::ivec3 &from, const glm::ivec3 &to, bool erase) {
	if (node.locked() || _sceneMgr->isLocked() || _sceneMgr->modifier().isLocked()) {
		return;
	}
	ModifierVolumeWrapper wrapper(node, erase ? ModifierType::Erase : ModifierType::Override,
								  _sceneMgr->modifier().selectBrush().box3D().selectionRegion());
	const glm::ivec3 delta = glm::abs(to - from);
	const int steps = core_max(1, core_max(delta.x, core_max(delta.y, delta.z)));
	for (int step = 0; step <= steps; ++step) {
		const glm::ivec3 pos = glm::round(glm::mix(glm::vec3(from), glm::vec3(to), (float)step / steps));
		if (!node.volume()->region().containsPoint(pos)) {
			continue;
		}
		voxel::Voxel value = erase ? voxel::Voxel() : _sceneMgr->modifier().cursorVoxel();
		const voxel::Voxel &previous = node.volume()->voxel(pos);
		value.setFlags(previous.getFlags());
		if (!(value == previous)) {
			wrapper.setVoxel(pos.x, pos.y, pos.z, value);
		}
	}
	if (wrapper.dirtyRegion().isValid()) {
		if (_stroke) {
			if (_strokeRegion.isValid()) {
				_strokeRegion.accumulate(wrapper.dirtyRegion());
			} else {
				_strokeRegion = wrapper.dirtyRegion();
			}
		}
		_sceneMgr->modified(node.uuid(), wrapper.dirtyRegion(),
							_stroke ? SceneModifiedFlags::NoUndo : SceneModifiedFlags::All);
	}
}

void SlicePanel::drawView(int view, scenegraph::SceneGraphNode &node, const ImVec2 &size) {
	const char *names[] = {_("Back"), _("Right"), _("Top")};
	const char *axes[] = {"Z", "X", "Y"};
	ImGui::PushID(view);
	if (!ImGui::BeginChild("view", size, ImGuiChildFlags_Borders)) {
		ImGui::EndChild();
		ImGui::PopID();
		return;
	}
	const voxel::RawVolume &volume = *node.volume();
	const voxel::Region &region = volume.region();
	const glm::ivec3 lower = region.getLowerCorner();
	const glm::ivec3 upper = region.getUpperCorner();
	const glm::ivec3 dimensions = region.getDimensionsInVoxels();
	ImGui::TextUnformatted(names[view]);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(core_max(60.0f, ImGui::GetContentRegionAvail().x - 55.0f));
	if (ImGui::SliderInt(axes[view], &_slices[view], lower[DepthAxis[view]], upper[DepthAxis[view]])) {
		finishStroke();
		_target[DepthAxis[view]] = _slices[view];
	}
	ImGui::SameLine();
	if (ImGui::SmallButton(_("Fit"))) {
		_zoom[view] = 1.0f;
		_pan[view] = ImVec2(0, 0);
	}
	const ImVec2 canvas = ImGui::GetCursorScreenPos();
	const ImVec2 available = ImGui::GetContentRegionAvail();
	if (available.x <= 0 || available.y <= 0) {
		ImGui::EndChild();
		ImGui::PopID();
		return;
	}
	const char *canvasIds[] = {"backcanvas", "rightcanvas", "topcanvas"};
	ImGui::InvisibleButton(canvasIds[view], available,
						   ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight |
							   ImGuiButtonFlags_MouseButtonMiddle);
	const bool hovered = ImGui::IsItemHovered();
	const ImGuiIO &io = ImGui::GetIO();
	if (hovered && io.MouseWheel != 0.0f) {
		_zoom[view] = glm::clamp(_zoom[view] * (io.MouseWheel > 0 ? 1.2f : 1.0f / 1.2f), 0.25f, 64.0f);
	}
	if (hovered && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
		_pan[view].x += io.MouseDelta.x;
		_pan[view].y += io.MouseDelta.y;
	}
	const int width = dimensions[HorizontalAxis[view]];
	const int height = dimensions[VerticalAxis[view]];
	const float cell = core_max(1.0f, core_min(available.x / width, available.y / height) * _zoom[view]);
	const ImVec2 origin(canvas.x + (available.x - width * cell) * 0.5f + _pan[view].x,
						canvas.y + (available.y - height * cell) * 0.5f + _pan[view].y);
	const int mouseU = (int)glm::floor((io.MousePos.x - origin.x) / cell);
	const int mouseV = (int)glm::floor((io.MousePos.y - origin.y) / cell);
	const bool inside = hovered && mouseU >= 0 && mouseV >= 0 && mouseU < width && mouseV < height;
	if (inside) {
		const glm::ivec3 pos = voxelPosition(view, region, _slices[view], mouseU, mouseV);
		_target = pos;
		ImGui::SetTooltip("%s: %i, %i, %i", names[view], pos.x, pos.y, pos.z);
		if (io.KeyAlt && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			voxel::Voxel voxel = volume.voxel(pos);
			if (!voxel::isAir(voxel.getMaterial())) {
				voxel.setFlags(0);
				_sceneMgr->modifier().setCursorVoxel(voxel);
			}
		} else if (!io.KeyAlt && (ImGui::IsItemActive() || (_stroke && _strokeView == view)) &&
				   (ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseDown(ImGuiMouseButton_Right))) {
			if (!_stroke) {
				_stroke = true;
				_strokeNodeUUID = node.uuid();
				_strokeView = view;
			}
			if (_strokeView == view) {
				const glm::ivec3 start = _lastPosValid ? _lastPos : pos;
				editLine(node, start, pos, ImGui::IsMouseDown(ImGuiMouseButton_Right));
				_lastPos = pos;
				_lastPosValid = true;
			}
		}
	} else if (_strokeView == view) {
		_lastPosValid = false;
	}
	ImDrawList *draw = ImGui::GetWindowDrawList();
	draw->PushClipRect(canvas, ImVec2(canvas.x + available.x, canvas.y + available.y), true);
	_targetViews[view] = {draw, origin, draw->GetClipRectMin(), draw->GetClipRectMax(), cell};
	draw->AddRectFilled(canvas, ImVec2(canvas.x + available.x, canvas.y + available.y), IM_COL32(32, 35, 40, 255));
	const int firstU = glm::clamp((int)glm::floor((canvas.x - origin.x) / cell), 0, width);
	const int firstV = glm::clamp((int)glm::floor((canvas.y - origin.y) / cell), 0, height);
	const int lastU = glm::clamp((int)glm::ceil((canvas.x + available.x - origin.x) / cell), 0, width);
	const int lastV = glm::clamp((int)glm::ceil((canvas.y + available.y - origin.y) / cell), 0, height);
	for (int v = firstV; v < lastV; ++v) {
		for (int u = firstU; u < lastU; ++u) {
			glm::ivec3 pos = voxelPosition(view, region, _slices[view], u, v);
			voxel::Voxel voxel = volume.voxel(pos);
			const bool foreground = !voxel::isAir(voxel.getMaterial());
			if (_background && !foreground) {
				while (--pos[DepthAxis[view]] >= lower[DepthAxis[view]]) {
					voxel = volume.voxel(pos);
					if (!voxel::isAir(voxel.getMaterial())) {
						break;
					}
				}
			}
			const ImVec2 min(origin.x + u * cell, origin.y + v * cell);
			const ImVec2 max(min.x + cell, min.y + cell);
			if (!voxel::isAir(voxel.getMaterial())) {
				const color::RGBA color = node.palette().color(voxel.getColor());
				draw->AddRectFilled(min, max, IM_COL32(color.r, color.g, color.b, foreground ? 255 : 85));
			}
			if (_grid && cell >= 5.0f) {
				draw->AddRect(min, max, foreground ? IM_COL32(210, 65, 65, 190) : IM_COL32(95, 100, 110, 80));
			}
		}
	}
	draw->PopClipRect();
	ImGui::EndChild();
	ImGui::PopID();
}

void SlicePanel::update(const char *id) {
	if (_stroke && (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::IsMouseDown(ImGuiMouseButton_Right))) {
		finishStroke();
	}
	static ui::ScopedPanel panel(cfg::VoxEditShowSlices);
	if (!panel.isOpen()) {
		finishStroke();
		return;
	}
	const core::String &title = makeTitle(ICON_LC_LAYERS_2, _("2D Views"), id);
	if (ui::ScopedPanel::Scope scope = panel.begin(title.c_str(), ImGuiWindowFlags_NoFocusOnAppearing)) {
		scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(_sceneMgr->activeNode());
		if (node == nullptr || !node->isModelNode() || node->volume() == nullptr) {
			finishStroke();
			ImGui::TextUnformatted(_("Select a model node to edit its slices."));
			return;
		}
		const voxel::Region &region = node->volume()->region();
		if (_nodeUUID != node->uuid()) {
			finishStroke();
			_nodeUUID = node->uuid();
			_target = region.getCenter();
			for (int view = 0; view < 3; ++view) {
				_slices[view] = region.getCenter()[DepthAxis[view]];
				_zoom[view] = 1.0f;
				_pan[view] = ImVec2(0, 0);
			}
		}
		_target = glm::clamp(_target, region.getLowerCorner(), region.getUpperCorner());
		for (int view = 0; view < 3; ++view) {
			_slices[view] = glm::clamp(_slices[view], region.getLowerCorner()[DepthAxis[view]],
									   region.getUpperCorner()[DepthAxis[view]]);
		}
		ImGui::Checkbox(_("Background"), &_background);
		ImGui::TooltipTextUnformatted(_("Show voxels behind the current slice with reduced opacity."));
		ImGui::SameLine();
		ImGui::Checkbox(_("Grid"), &_grid);
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		ImGui::TooltipTextUnformatted(_("Left: draw\nRight: erase\nAlt+Left: pick\nWheel: zoom\nMiddle: pan"));
		ImGui::TextUnformatted(node->name().c_str());
		const ImVec2 avail = ImGui::GetContentRegionAvail();
		const float gap = ImGui::GetStyle().ItemSpacing.x;
		const float width = core_max(1.0f, (avail.x - gap) * 0.5f);
		const float height = core_max(1.0f, avail.y);
		for (TargetView &targetView : _targetViews) {
			targetView.draw = nullptr;
		}
		drawView(0, *node, ImVec2(width, height));
		ImGui::SameLine();
		ImGui::BeginGroup();
		drawView(1, *node, ImVec2(width, core_max(1.0f, (height - ImGui::GetStyle().ItemSpacing.y) * 0.5f)));
		drawView(2, *node, ImVec2(width, core_max(1.0f, (height - ImGui::GetStyle().ItemSpacing.y) * 0.5f)));
		ImGui::EndGroup();
		// Draw after all views have handled input so each cross shows the same target this frame.
		drawTarget(region);
	} else {
		finishStroke();
	}
}

} // namespace voxedit
