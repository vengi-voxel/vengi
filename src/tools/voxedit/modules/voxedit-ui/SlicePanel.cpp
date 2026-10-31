/**
 * @file
 */

#include "SlicePanel.h"
#include "app/I18N.h"
#include "color/ColorUtil.h"
#include "core/Common.h"
#include "core/StandardLib.h"
#include "palette/Palette.h"
#include "palette/PaletteView.h"
#include "ui/IMGUIEx.h"
#include "ui/IconsLucide.h"
#include "ui/ScopedPanel.h"
#include "ui/Style.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace voxedit {

// Back: +Z looking towards -Z. Right: +X towards -X. Top: +Y towards -Y.
static constexpr SliceView AllSliceViews[] = {SliceView::Back, SliceView::Right, SliceView::Top};
static constexpr math::Axis DepthAxis[] = {math::Axis::Z, math::Axis::X, math::Axis::Y};
static constexpr math::Axis HorizontalAxis[] = {math::Axis::X, math::Axis::Z, math::Axis::X};
static constexpr math::Axis VerticalAxis[] = {math::Axis::Y, math::Axis::Y, math::Axis::Z};

static inline int viewIndex(SliceView view) {
	return (int)view;
}

static ImU32 themeColor(style::StyleColor id, float alpha = 1.0f) {
	return ImGui::GetColorU32(ImVec4(color::alpha(style::color(id), alpha)));
}

static void viewUVToPlane(SliceView view, int u, int v, int visW, int visH, int &a, int &b) {
	if (view == SliceView::Right) {
		a = visH - 1 - v;
		b = visW - 1 - u;
	} else if (view == SliceView::Top) {
		a = u;
		b = v;
	} else {
		a = u;
		b = visH - 1 - v;
	}
}

SlicePanel::SlicePanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr) : Super(app, "slices"), _sceneMgr(sceneMgr) {
}

SlicePanel::~SlicePanel() {
	_sceneMgr->modifier().setSliceMode(false);
}

glm::ivec3 SlicePanel::voxelPosition(SliceView view, const voxel::Region &region, int slice, int u, int v) {
	const int i = viewIndex(view);
	glm::ivec3 pos = region.getLowerCorner();
	pos[math::getIndexForAxis(DepthAxis[i])] = slice;
	pos[math::getIndexForAxis(HorizontalAxis[i])] =
		view == SliceView::Right ? region.getUpperCorner().z - u : pos[math::getIndexForAxis(HorizontalAxis[i])] + u;
	pos[math::getIndexForAxis(VerticalAxis[i])] = view == SliceView::Top ? pos.z + v : region.getUpperCorner().y - v;
	return pos;
}

glm::ivec2 SlicePanel::viewPosition(SliceView view, const voxel::Region &region, const glm::ivec3 &pos) {
	const int u = view == SliceView::Right ? region.getUpperCorner().z - pos.z : pos.x - region.getLowerCorner().x;
	const int v = view == SliceView::Top ? pos.z - region.getLowerCorner().z : region.getUpperCorner().y - pos.y;
	return glm::ivec2(u, v);
}

void SlicePanel::drawTarget(const voxel::Region &region) {
	if (_gizmoHovered || _gizmoView != SliceView::None) {
		return;
	}
	for (const SliceView view : AllSliceViews) {
		const TargetView &targetView = _targetViews[viewIndex(view)];
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
			const ImU32 color =
				pass == 0 ? ImGui::GetColorU32(ImGuiCol_WindowBg, 0.9f) : themeColor(style::ColorBrushGizmoLine);
			const float thickness = core_max(1.0f, ImGui::Size(pass == 0 ? 0.375f : 0.125f));
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

voxel::Region SlicePanel::sliceSelection(const scenegraph::SceneGraphNode &node, SliceView view) const {
	voxel::Region selected = voxel::Region::InvalidRegion;
	const voxel::Region &region = node.region();
	const glm::ivec3 dimensions = region.getDimensionsInVoxels();
	const int i = viewIndex(view);
	for (int v = 0; v < dimensions[math::getIndexForAxis(VerticalAxis[i])]; ++v) {
		for (int u = 0; u < dimensions[math::getIndexForAxis(HorizontalAxis[i])]; ++u) {
			const glm::ivec3 pos = voxelPosition(view, region, _slices[i], u, v);
			const voxel::Voxel &voxel = node.volume()->voxel(pos);
			if (!voxel::isAir(voxel.getMaterial()) && (voxel.getFlags() & voxel::FlagOutline)) {
				if (selected.isValid()) {
					selected.accumulate(pos);
				} else {
					selected = voxel::Region(pos, pos);
				}
			}
		}
	}
	return selected;
}

void SlicePanel::moveSelection(scenegraph::SceneGraphNode &node, SliceView view, const glm::ivec3 &offset) {
	if (node.locked() || _sceneMgr->isLocked() || _sceneMgr->modifier().isLocked() || offset == glm::ivec3(0) ||
		offset[math::getIndexForAxis(DepthAxis[viewIndex(view)])] != 0) {
		return;
	}
	const voxel::Region selected = sliceSelection(node, view);
	if (!selected.isValid() || !node.region().containsRegion(voxel::Region(selected.getLowerCorner() + offset,
																		   selected.getUpperCorner() + offset))) {
		return;
	}
	core::DynamicArray<glm::ivec3> positions;
	core::DynamicArray<voxel::Voxel> voxels;
	for (int z = selected.getLowerZ(); z <= selected.getUpperZ(); ++z) {
		for (int y = selected.getLowerY(); y <= selected.getUpperY(); ++y) {
			for (int x = selected.getLowerX(); x <= selected.getUpperX(); ++x) {
				const voxel::Voxel voxel = node.volume()->voxel(x, y, z);
				if (!voxel::isAir(voxel.getMaterial()) && (voxel.getFlags() & voxel::FlagOutline)) {
					positions.push_back(glm::ivec3(x, y, z));
					voxels.push_back(voxel);
					node.volume()->setVoxel(x, y, z, voxel::Voxel());
				}
			}
		}
	}
	for (size_t i = 0; i < positions.size(); ++i) {
		node.volume()->setVoxel(positions[i] + offset, voxels[i]);
	}
	voxel::Region dirty = selected;
	dirty.accumulate(voxel::Region(selected.getLowerCorner() + offset, selected.getUpperCorner() + offset));
	_sceneMgr->modifier().selectBrush().box3D().setSelectionRegion(voxel::Region::InvalidRegion);
	_sceneMgr->modified(node.uuid(), dirty);
}

voxel::Voxel SlicePanel::displayVoxel(const voxel::RawVolume &volume, const glm::ivec3 &pos) const {
	if (_gizmoPreview && _gizmoPreview->region().containsPoint(pos)) {
		return _gizmoPreview->voxel(pos);
	}
	return volume.voxel(pos);
}

void SlicePanel::updateGizmoPreview(scenegraph::SceneGraphNode &node) {
	if (_gizmoView == SliceView::None) {
		_gizmoPreview = nullptr;
		return;
	}
	const SliceView view = _gizmoView;
	const int i = viewIndex(view);
	const ImVec2 mouse = ImGui::GetIO().MousePos;
	const int du = _gizmoAxis == HorizontalAxis[i] ? (int)glm::round((mouse.x - _gizmoStart.x) / _gizmoCell) : 0;
	const int dv = _gizmoAxis == VerticalAxis[i] ? (int)glm::round((mouse.y - _gizmoStart.y) / _gizmoCell) : 0;
	const glm::ivec3 offset = voxelPosition(view, node.region(), _slices[i], du, dv) -
							  voxelPosition(view, node.region(), _slices[i], 0, 0);
	if (!_sceneMgr->modifier().sliceActionPressed()) {
		moveSelection(node, view, offset);
		_gizmoView = SliceView::None;
		_gizmoOffset = glm::ivec3(0);
		_gizmoPreview = nullptr;
		return;
	}
	if (_gizmoPreview && offset == _gizmoOffset) {
		return;
	}
	_gizmoOffset = offset;
	glm::ivec3 lo = node.region().getLowerCorner();
	glm::ivec3 hi = node.region().getUpperCorner();
	lo[math::getIndexForAxis(DepthAxis[i])] = hi[math::getIndexForAxis(DepthAxis[i])] = _slices[i];
	if (!_gizmoPreview) {
		_gizmoPreview = core::make_shared<voxel::RawVolume>(*node.volume(), voxel::Region(lo, hi));
	} else {
		_gizmoPreview->copyInto(*node.volume());
	}
	const voxel::Region selected = sliceSelection(node, view);
	if (!selected.isValid() || !node.region().containsRegion(voxel::Region(selected.getLowerCorner() + offset,
																		   selected.getUpperCorner() + offset))) {
		return;
	}
	// Stage both the erased source and moved cells without changing scene data or undo history.
	for (int z = selected.getLowerZ(); z <= selected.getUpperZ(); ++z) {
		for (int y = selected.getLowerY(); y <= selected.getUpperY(); ++y) {
			for (int x = selected.getLowerX(); x <= selected.getUpperX(); ++x) {
				const voxel::Voxel &voxel = node.volume()->voxel(x, y, z);
				if (voxel.getFlags() & voxel::FlagOutline) {
					_gizmoPreview->setVoxel(x, y, z, voxel::Voxel());
				}
			}
		}
	}
	for (int z = selected.getLowerZ(); z <= selected.getUpperZ(); ++z) {
		for (int y = selected.getLowerY(); y <= selected.getUpperY(); ++y) {
			for (int x = selected.getLowerX(); x <= selected.getUpperX(); ++x) {
				const voxel::Voxel &voxel = node.volume()->voxel(x, y, z);
				if (!voxel::isAir(voxel.getMaterial()) && (voxel.getFlags() & voxel::FlagOutline)) {
					_gizmoPreview->setVoxel(glm::ivec3(x, y, z) + offset, voxel);
				}
			}
		}
	}
}

bool SlicePanel::selectionGizmo(SliceView view, scenegraph::SceneGraphNode &node, const ImVec2 &origin, float cell,
								bool hovered) {
	if (_gizmoView != SliceView::None && _gizmoView != view) {
		return false;
	}
	const BrushType brush = _sceneMgr->modifier().brushType();
	if (brush != BrushType::Select && brush != BrushType::Transform) {
		return false;
	}
	const voxel::Region selected = sliceSelection(node, view);
	if (!selected.isValid()) {
		return false;
	}
	const int i = viewIndex(view);
	const glm::ivec2 uv =
		viewPosition(view, node.region(), selected.getCenter() + (_gizmoView == view ? _gizmoOffset : glm::ivec3(0)));
	const ImVec2 center(origin.x + (uv.x + 0.5f) * cell, origin.y + (uv.y + 0.5f) * cell);
	const ImVec2 mouse = ImGui::GetIO().MousePos;
	const bool pressed = _sceneMgr->modifier().sliceActionPressed();
	bool hit = false;
	const math::Axis axes[] = {HorizontalAxis[i], VerticalAxis[i]};
	for (math::Axis axis : axes) {
		const ImVec2 tip(center.x + (axis == HorizontalAxis[i] ? ImGui::Size(3.5f) : 0.0f),
						 center.y - (axis == VerticalAxis[i] ? ImGui::Size(3.5f) : 0.0f));
		const bool over =
			hovered && glm::abs(mouse.x - tip.x) <= ImGui::Size(1.0f) && glm::abs(mouse.y - tip.y) <= ImGui::Size(1.0f);
		hit |= over;
		if (over && pressed && _gizmoView == SliceView::None && !node.locked() && !_sceneMgr->isLocked()) {
			_gizmoView = view;
			_gizmoAxis = axis;
			_gizmoStart = mouse;
			_gizmoCell = cell;
			_gizmoOffset = glm::ivec3(0);
			_gizmoPreview = nullptr;
		}
		ImDrawList *draw = ImGui::GetWindowDrawList();

		const style::StyleColor axisColor = axis == math::Axis::X	? style::ColorAxisX
											: axis == math::Axis::Y ? style::ColorAxisY
																	: style::ColorAxisZ;
		const ImU32 color = themeColor(over ? style::ColorBrushGizmoLine : axisColor);
		draw->AddLine(center, tip, ImGui::GetColorU32(ImGuiCol_WindowBg), ImGui::Size(0.625f));
		draw->AddLine(center, tip, color, ImGui::Size(0.375f));
		draw->AddCircleFilled(tip, ImGui::Size(0.625f), color);
	}
	if (hit || _gizmoView == view) {
		_gizmoHovered = true;
		_sceneMgr->modifier().setBrushGizmoActive(true);
	}
	if (_gizmoView == view) {
		const glm::ivec2 lo = viewPosition(view, node.region(), selected.getLowerCorner() + _gizmoOffset);
		const glm::ivec2 hi = viewPosition(view, node.region(), selected.getUpperCorner() + _gizmoOffset);
		const glm::ivec2 min = glm::min(lo, hi);
		const glm::ivec2 max = glm::max(lo, hi) + glm::ivec2(1);
		ImGui::GetWindowDrawList()->AddRect(ImVec2(origin.x + min.x * cell, origin.y + min.y * cell),
											ImVec2(origin.x + max.x * cell, origin.y + max.y * cell),
											themeColor(style::ColorHighlightArea), 0.0f, ImGui::Size(0.25f));
		return true;
	}
	return hit;
}

void SlicePanel::drawView(SliceView view, scenegraph::SceneGraphNode &node, const ImVec2 &size) {
	const int i = viewIndex(view);
	const char *names[] = {_("Back"), _("Right"), _("Top")};
	const char *axes[] = {"Z", "X", "Y"};
	ImGui::PushID(i);
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
	const ImGuiStyle &uiStyle = ImGui::GetStyle();
	const float fitWidth = ImGui::CalcTextSize(_("Fit")).x + uiStyle.FramePadding.x * 2.0f;
	const float labelWidth = ImGui::CalcTextSize(axes[i]).x + uiStyle.ItemInnerSpacing.x;
	const float controlsWidth = fitWidth + labelWidth + uiStyle.ItemSpacing.x;
	const float headerWidth = ImGui::GetContentRegionAvail().x;
	const float titleWidth = ImGui::CalcTextSize(names[i]).x;
	ImGui::TextUnformatted(names[i]);
	if (headerWidth >= titleWidth + uiStyle.ItemSpacing.x + ImGui::Size(4.0f) + controlsWidth) {
		ImGui::SameLine();
	}
	ImGui::BeginDisabled(_gizmoView != SliceView::None);
	ImGui::SetNextItemWidth(core_max(ImGui::Size(1.0f), ImGui::GetContentRegionAvail().x - controlsWidth));
	if (ImGui::SliderInt(axes[i], &_slices[i], lower[math::getIndexForAxis(DepthAxis[i])],
						 upper[math::getIndexForAxis(DepthAxis[i])])) {
		_target[math::getIndexForAxis(DepthAxis[i])] = _slices[i];
	}
	ImGui::SameLine();
	if (ImGui::SmallButton(_("Fit"))) {
		_zoom[i] = 1.0f;
		_pan[i] = ImVec2(0, 0);
	}
	ImGui::EndDisabled();
	const ImVec2 canvas = ImGui::GetCursorScreenPos();
	const ImVec2 available = ImGui::GetContentRegionAvail();
	if (available.x <= 0 || available.y <= 0) {
		ImGui::EndChild();
		ImGui::PopID();
		return;
	}
	const char *canvasIds[] = {"backcanvas", "rightcanvas", "topcanvas"};
	ImGui::InvisibleButton(canvasIds[i], available,
						   ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight |
							   ImGuiButtonFlags_MouseButtonMiddle);
	const bool hovered = ImGui::IsItemHovered();
	const ImGuiIO &io = ImGui::GetIO();
	if (hovered && _gizmoView == SliceView::None && io.MouseWheel != 0.0f) {
		_zoom[i] = glm::clamp(_zoom[i] * (io.MouseWheel > 0 ? 1.2f : 1.0f / 1.2f), 0.25f, 64.0f);
	}
	if (hovered && _gizmoView == SliceView::None && ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
		_pan[i].x += io.MouseDelta.x;
		_pan[i].y += io.MouseDelta.y;
	}
	const int width = dimensions[math::getIndexForAxis(HorizontalAxis[i])];
	const int height = dimensions[math::getIndexForAxis(VerticalAxis[i])];
	const float cell = core_max(ImGui::Size(0.125f), core_min(available.x / width, available.y / height) * _zoom[i]);
	const ImVec2 origin(canvas.x + (available.x - width * cell) * 0.5f + _pan[i].x,
						canvas.y + (available.y - height * cell) * 0.5f + _pan[i].y);
	const int mouseU = (int)glm::floor((io.MousePos.x - origin.x) / cell);
	const int mouseV = (int)glm::floor((io.MousePos.y - origin.y) / cell);
	const bool inside = hovered && mouseU >= 0 && mouseV >= 0 && mouseU < width && mouseV < height;

	ImDrawList *draw = ImGui::GetWindowDrawList();
	draw->PushClipRect(canvas, ImVec2(canvas.x + available.x, canvas.y + available.y), true);
	_targetViews[i] = {draw, origin, draw->GetClipRectMin(), draw->GetClipRectMax(), cell};
	draw->AddRectFilled(canvas, ImVec2(canvas.x + available.x, canvas.y + available.y),
						ImGui::GetColorU32(ImGuiCol_FrameBg));
	const int firstU = glm::clamp((int)glm::floor((canvas.x - origin.x) / cell), 0, width);
	const int firstV = glm::clamp((int)glm::floor((canvas.y - origin.y) / cell), 0, height);
	const int lastU = glm::clamp((int)glm::ceil((canvas.x + available.x - origin.x) / cell), 0, width);
	const int lastV = glm::clamp((int)glm::ceil((canvas.y + available.y - origin.y) / cell), 0, height);
	const bool showGrid = _grid && cell >= ImGui::Size(0.625f);
	const math::Axis depth = DepthAxis[i];
	const int depthAxis = math::getIndexForAxis(depth);
	const float fillInset = showGrid ? 0.5f : 0.0f;
	if (showGrid && firstU < lastU && firstV < lastV) {
		const ImU32 gridColor = themeColor(style::ColorGridBorder, 0.3f);
		const ImVec2 min(origin.x + firstU * cell, origin.y + firstV * cell);
		const ImVec2 max(origin.x + lastU * cell, origin.y + lastV * cell);
		for (int u = firstU; u <= lastU; ++u) {
			const float x = origin.x + u * cell;
			draw->AddLine(ImVec2(x, min.y), ImVec2(x, max.y), gridColor);
		}
		for (int v = firstV; v <= lastV; ++v) {
			const float y = origin.y + v * cell;
			draw->AddLine(ImVec2(min.x, y), ImVec2(max.x, y), gridColor);
		}
	}
	const ImU32 selectionColor = themeColor(style::ColorHighlightArea);
	const float selectionThickness = ImGui::Size(0.25f);
	const ImU32 foregroundAlpha = ImGui::GetColorU32(ImVec4(0, 0, 0, 1));
	const ImU32 backgroundAlpha = ImGui::GetColorU32(ImVec4(0, 0, 0, 85.0f / 255.0f));
	const int visW = lastU - firstU;
	const int visH = lastV - firstV;
	if (visW > 0 && visH > 0) {
		const int aAxis = math::getIndexForAxis(math::getPlaneAxisA(depth));
		const int bAxis = math::getIndexForAxis(math::getPlaneAxisB(depth));
		const glm::ivec3 p00 = voxelPosition(view, region, _slices[i], firstU, firstV);
		const glm::ivec3 p11 = voxelPosition(view, region, _slices[i], lastU - 1, lastV - 1);
		const int minA = core_min(p00[aAxis], p11[aAxis]);
		const int maxA = core_max(p00[aAxis], p11[aAxis]);
		const int minB = core_min(p00[bAxis], p11[bAxis]);
		const int maxB = core_max(p00[bAxis], p11[bAxis]);
		const int aCount = maxA - minA + 1;
		const int bCount = maxB - minB + 1;
		const int cellCount = aCount * bCount;
		_sliceCells.resize((size_t)cellCount);
		_sliceState.resize((size_t)cellCount);
		core_memset(&_sliceState[0], 0, (size_t)cellCount);
		voxel::Voxel *cells = &_sliceCells[0];
		uint8_t *state = &_sliceState[0];

		const voxel::Voxel *data = volume.voxels();
		const bool preview = _gizmoPreview != nullptr;
		const int64_t strides[3] = {1, (int64_t)volume.width(), region.stride()};
		glm::ivec3 start(0);
		start[depthAxis] = _slices[i];
		start[aAxis] = minA;
		start[bAxis] = minB;
		const int64_t row0 = region.index(start);
		for (int b = 0; b < bCount; ++b) {
			int64_t idx = row0 + (int64_t)b * strides[bAxis];
			glm::ivec3 pos = start;
			pos[bAxis] = minB + b;
			for (int a = 0; a < aCount; ++a, idx += strides[aAxis]) {
				voxel::Voxel voxel = data[idx];
				if (preview) {
					pos[aAxis] = minA + a;
					if (_gizmoPreview->region().containsPoint(pos)) {
						voxel = _gizmoPreview->voxel(pos);
					}
				}
				const int cellIndex = b * aCount + a;
				cells[cellIndex] = voxel;
				if (!voxel::isAir(voxel.getMaterial())) {
					state[cellIndex] = (voxel.getFlags() & voxel::FlagOutline) ? 2 : 1;
				}
			}
		}
		if (_background) {
			volume.fillBehindPlane(DepthAxis[i], _slices[i], minA, maxA, minB, maxB, cells);
		}

		const palette::Palette &palette = node.palette();
		if (palette.hash() != _lastPaletteHash) {
			for (int paletteIdx = 0; paletteIdx < palette::PaletteMaxColors; ++paletteIdx) {
				const color::RGBA color = palette.color((uint8_t)paletteIdx);
				const ImU32 rgb = IM_COL32(color.r, color.g, color.b, 0);
				_fgLut[paletteIdx] = rgb | foregroundAlpha;
				_bgLut[paletteIdx] = rgb | backgroundAlpha;
			}
			_lastPaletteHash = palette.hash();
		}
		draw->PrimReserve(cellCount * 6, cellCount * 4);
		int emitted = 0;
		for (int v = 0; v < visH; ++v) {
			for (int u = 0; u < visW; ++u) {
				int a;
				int b;
				viewUVToPlane(view, u, v, visW, visH, a, b);
				const int cellIndex = b * aCount + a;
				const voxel::Voxel &voxel = cells[cellIndex];
				if (voxel::isAir(voxel.getMaterial())) {
					continue;
				}
				const ImU32 fill = state[cellIndex] != 0 ? _fgLut[voxel.getColor()] : _bgLut[voxel.getColor()];
				if ((fill & IM_COL32_A_MASK) == 0) {
					continue;
				}
				const float x = origin.x + (float)(firstU + u) * cell;
				const float y = origin.y + (float)(firstV + v) * cell;
				draw->PrimRect(ImVec2(x + fillInset, y + fillInset), ImVec2(x + cell - fillInset, y + cell - fillInset),
							   fill);
				++emitted;
			}
		}
		draw->PrimUnreserve((cellCount - emitted) * 6, (cellCount - emitted) * 4);

		for (int v = 0; v < visH; ++v) {
			for (int u = 0; u < visW; ++u) {
				int a;
				int b;
				viewUVToPlane(view, u, v, visW, visH, a, b);
				if (state[b * aCount + a] != 2) {
					continue;
				}
				const float x = origin.x + (float)(firstU + u) * cell;
				const float y = origin.y + (float)(firstV + v) * cell;
				draw->AddRect(ImVec2(x, y), ImVec2(x + cell, y + cell), selectionColor, 0.0f, selectionThickness);
			}
		}
	}
	selectionGizmo(view, node, origin, cell, hovered);
	Modifier &modifier = _sceneMgr->modifier();
	const bool action = modifier.sliceActionPressed() || modifier.sliceDeletePressed();
	if ((inside || _gizmoView == view) && (!action || _inputView == SliceView::None || _inputView == view)) {
		_hovered = true;
		_inputView = view;
		modifier.setSliceMode(true);
		glm::ivec3 lo = lower;
		glm::ivec3 hi = upper;
		lo[math::getIndexForAxis(DepthAxis[i])] = hi[math::getIndexForAxis(DepthAxis[i])] = _slices[i];
		modifier.setSliceRegion(voxel::Region(lo, hi));
		const glm::ivec3 pos = voxelPosition(view, region, _slices[i], glm::clamp(mouseU, 0, width - 1),
											 glm::clamp(mouseV, 0, height - 1));
		_target = pos;
		const voxel::FaceNames faces[] = {voxel::FaceNames::Back, voxel::FaceNames::Right, voxel::FaceNames::Top};
		modifier.setCursorPosition(pos, faces[i]);
		voxel::Voxel hit = volume.voxel(pos);
		hit.setFlags(0);
		modifier.setHitCursorVoxel(hit);
		modifier.setVoxelAtCursor(hit);
		// Screen-based brushes (such as Lasso) use the same orthographic mapping.
		video::Camera &camera = _cameras[i];
		camera.setMode(video::CameraMode::Orthogonal);
		const float scale = 1.0f / (camera.worldUnitsPerPixel() * cell);
		camera.setSize(glm::max(glm::ivec2(1), glm::ivec2(available.x * scale, available.y * scale)));
		glm::vec3 center(pos);
		center[math::getIndexForAxis(HorizontalAxis[i])] =
			view == SliceView::Right ? upper.z + 1.0f - (canvas.x + available.x * 0.5f - origin.x) / cell
									 : lower.x + (canvas.x + available.x * 0.5f - origin.x) / cell;
		center[math::getIndexForAxis(VerticalAxis[i])] =
			view == SliceView::Top ? lower.z + (canvas.y + available.y * 0.5f - origin.y) / cell
								   : upper.y + 1.0f - (canvas.y + available.y * 0.5f - origin.y) / cell;
		center[math::getIndexForAxis(DepthAxis[i])] = _slices[i] + 0.5f;
		glm::vec3 normal(0.0f);
		normal[math::getIndexForAxis(DepthAxis[i])] = 100.0f;
		camera.setWorldPosition(center + normal);
		camera.lookAt(center, view == SliceView::Top ? glm::vec3(0, 0, -1) : glm::vec3(0, 1, 0));
		camera.update(0.0);
		_sceneMgr->setActiveCamera(&camera, true);
		_sceneMgr->setMousePos((int)((io.MousePos.x - canvas.x) * scale), (int)((io.MousePos.y - canvas.y) * scale));
		ImGui::SetTooltip("%s: %i, %i, %i", names[i], pos.x, pos.y, pos.z);
	}

	draw->PopClipRect();
	ImGui::EndChild();
	ImGui::PopID();
}

void SlicePanel::update(const char *id) {
	_hovered = false;
	_gizmoHovered = false;
	Modifier &modifier = _sceneMgr->modifier();
	modifier.setSliceMode(false);
	if (!modifier.sliceActionPressed() && !modifier.sliceDeletePressed()) {
		_inputView = SliceView::None;
	}
	modifier.setBrushGizmoActive(false);
	static ui::ScopedPanel panel(cfg::VoxEditShowSlices);
	if (!panel.isOpen()) {
		_gizmoView = SliceView::None;
		_gizmoPreview = nullptr;
		return;
	}
	const core::String &title = makeTitle(ICON_LC_LAYERS_2, _("2D Views"), id);
	if (ui::ScopedPanel::Scope scope = panel.begin(title.c_str(), ImGuiWindowFlags_NoFocusOnAppearing)) {
		scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphNode(_sceneMgr->activeNode());
		if (node == nullptr || !node->isModelNode() || node->volume() == nullptr) {
			_gizmoView = SliceView::None;
			_gizmoPreview = nullptr;
			ImGui::TextUnformatted(_("Select a model node to edit its slices."));
			return;
		}
		const voxel::Region &region = node->volume()->region();
		if (_nodeUUID != node->uuid()) {
			_nodeUUID = node->uuid();
			_gizmoView = SliceView::None;
			_gizmoPreview = nullptr;
			_target = region.getCenter();
			for (const SliceView view : AllSliceViews) {
				const int i = viewIndex(view);
				_slices[i] = region.getCenter()[math::getIndexForAxis(DepthAxis[i])];
				_zoom[i] = 1.0f;
				_pan[i] = ImVec2(0, 0);
			}
		}
		_target = glm::clamp(_target, region.getLowerCorner(), region.getUpperCorner());
		for (const SliceView view : AllSliceViews) {
			const int i = viewIndex(view);
			_slices[i] = glm::clamp(_slices[i], region.getLowerCorner()[math::getIndexForAxis(DepthAxis[i])],
								   region.getUpperCorner()[math::getIndexForAxis(DepthAxis[i])]);
		}
		updateGizmoPreview(*node);
		ImGui::Checkbox(_("Background"), &_background);
		ImGui::TooltipTextUnformatted(_("Show voxels behind the current slice with reduced opacity."));
		ImGui::SameLine();
		ImGui::Checkbox(_("Grid"), &_grid);
		ImGui::TextUnformatted(node->name().c_str());
		const ImVec2 avail = ImGui::GetContentRegionAvail();
		const float gap = ImGui::GetStyle().ItemSpacing.x;
		const float width = core_max(ImGui::Size(0.125f), (avail.x - gap) * 0.5f);
		const float height = core_max(ImGui::Size(0.125f), avail.y);
		for (TargetView &targetView : _targetViews) {
			targetView.draw = nullptr;
		}
		drawView(SliceView::Back, *node, ImVec2(width, height));
		ImGui::SameLine();
		ImGui::BeginGroup();
		drawView(SliceView::Right, *node,
				 ImVec2(width, core_max(ImGui::Size(0.125f), (height - ImGui::GetStyle().ItemSpacing.y) * 0.5f)));
		drawView(SliceView::Top, *node,
				 ImVec2(width, core_max(ImGui::Size(0.125f), (height - ImGui::GetStyle().ItemSpacing.y) * 0.5f)));
		ImGui::EndGroup();
		// Draw after all views have handled input so each cross shows the same target this frame.
		drawTarget(region);
	} else {
		_gizmoView = SliceView::None;
		_gizmoPreview = nullptr;
	}
}

} // namespace voxedit
