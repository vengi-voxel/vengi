/**
 * @file
 */

#include "DrawShapeTool.h"
#include "Authoring.h"
#include "core/GLM.h"
#include "math/Math.h"
#include "voxel/Region.h"
#include "voxelutil/VisitInside.h"
#include <glm/geometric.hpp>

namespace voxedit {

static constexpr int MaxShapeCells = 250000;

static int64_t boxCount(const glm::ivec3 &lo, const glm::ivec3 &hi) {
	const int64_t dx = (int64_t)hi.x - (int64_t)lo.x + 1;
	const int64_t dy = (int64_t)hi.y - (int64_t)lo.y + 1;
	const int64_t dz = (int64_t)hi.z - (int64_t)lo.z + 1;
	if (dx <= 0 || dy <= 0 || dz <= 0) {
		return 0;
	}
	if (dx > MaxShapeCells || dy > MaxShapeCells || dz > MaxShapeCells || dx * dy > MaxShapeCells) {
		return (int64_t)MaxShapeCells + 1;
	}
	return dx * dy * dz;
}

template<typename INSIDE>
static bool collectShape(const glm::ivec3 &lo, const glm::ivec3 &hi, bool hollow, INSIDE inside,
						 core::DynamicArray<glm::ivec3> &out, core::String &error) {
	if (boxCount(lo, hi) > MaxShapeCells) {
		error = core::String::format("That shape is too big (over %i cells)", MaxShapeCells);
		return false;
	}
	voxelutil::visitInside(voxel::Region(lo, hi), hollow, inside, [&](int x, int y, int z) {
		out.push_back(glm::ivec3(x, y, z));
	});
	return true;
}

static bool readCorners(const json::Json &args, glm::ivec3 &lo, glm::ivec3 &hi, core::String &error) {
	if (!args.contains("from") || !args.contains("to")) {
		error = "from and to corners are required";
		return false;
	}
	glm::ivec3 from(0);
	glm::ivec3 to(0);
	if (!parseCell(args.get("from"), from, "from", error) || !parseCell(args.get("to"), to, "to", error)) {
		return false;
	}
	lo = glm::ivec3(core_min(from.x, to.x), core_min(from.y, to.y), core_min(from.z, to.z));
	hi = glm::ivec3(core_max(from.x, to.x), core_max(from.y, to.y), core_max(from.z, to.z));
	return true;
}

DrawShapeTool::DrawShapeTool() : Tool("voxedit_draw_shape") {
	_tool.set("description",
			  "Fill or carve a shape on a model node. box: inclusive from/to corners. sphere: center cell + radius "
			  "(radius 4 is 9 voxels across, centered on that cell). ellipsoid: fills the from/to box. "
			  "cylinder: base cell + height + radius, growing along axis (default y, up). line: from to to. "
			  "hollow keeps only the shell (a cell with a 6-neighbor outside the shape). "
			  "color is #RRGGBB or a palette index; -1 erases. nodeUUID defaults to the active model. Y is up.");

	json::Json inputSchema = objectSchema();
	json::Json required = json::Json::array();
	required.push("shape");
	required.push("color");
	inputSchema.set("required", required);

	json::Json properties = inputSchema.get("properties");
	json::Json nodeProp = propUUID();
	nodeProp.set("description", "Model node UUID. Defaults to the active model node.");
	properties.set("nodeUUID", nodeProp);

	json::Json shape = propTypeDescription("string", "Shape to draw");
	json::Json shapeEnum = json::Json::array();
	shapeEnum.push("box");
	shapeEnum.push("sphere");
	shapeEnum.push("ellipsoid");
	shapeEnum.push("cylinder");
	shapeEnum.push("line");
	shape.set("enum", shapeEnum);
	properties.set("shape", shape);
	json::Json color = json::Json::object();
	color.set("description", "#RRGGBB or a palette index. -1 erases. A new hex color is added to the palette.");
	json::Json colorOneOf = json::Json::array();
	colorOneOf.push(propTypeDescription("string", "#RRGGBB"));
	colorOneOf.push(propTypeDescription("integer", "Palette index. -1 erases."));
	color.set("oneOf", colorOneOf);
	properties.set("color", color);
	properties.set("from", propTypeDescription("array", "[x, y, z] inclusive corner for box, ellipsoid and line"));
	properties.set("to", propTypeDescription("array", "[x, y, z] inclusive corner for box, ellipsoid and line"));
	properties.set("center", propTypeDescription("array", "[x, y, z] center cell for sphere"));
	properties.set("radius", propTypeDescription("number", "Radius in voxels for sphere and cylinder"));
	properties.set("base", propTypeDescription("array", "[x, y, z] base cell for cylinder"));
	properties.set("height", propTypeDescription("integer", "Cylinder length along axis, in voxels. Default 1."));
	json::Json axis = propTypeDescription("string", "Cylinder axis. Default y.");
	json::Json axisEnum = json::Json::array();
	axisEnum.push("x");
	axisEnum.push("y");
	axisEnum.push("z");
	axis.set("enum", axisEnum);
	axis.set("default", "y");
	properties.set("axis", axis);
	json::Json hollow = propTypeDescription("boolean", "Keep only the shell. Default false.");
	hollow.set("default", false);
	properties.set("hollow", hollow);
	_tool.set("inputSchema", core::move(inputSchema));
}

bool DrawShapeTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	core::String error;
	scenegraph::SceneGraphNode *node = authoringNode(ctx, args, error);
	if (node == nullptr) {
		return ctx.result(id, error, true);
	}
	if (!args.contains("shape") || !args.contains("color")) {
		return ctx.result(id, "shape and color are required", true);
	}
	PaletteUse use;
	markUsedPaletteSlots(*node->volume(), use);
	AuthoringColor color;
	if (!resolveAuthoringColor(ctx, *node, args.get("color"), use, color, error)) {
		return ctx.result(id, error, true);
	}
	const bool hollow = args.boolVal("hollow", false);
	const core::String shape = args.strVal("shape", "").toLower();
	core::DynamicArray<glm::ivec3> positions;

	if (shape == "box") {
		glm::ivec3 lo(0);
		glm::ivec3 hi(0);
		if (!readCorners(args, lo, hi, error)) {
			return ctx.result(id, error, true);
		}
		auto inside = [lo, hi](int x, int y, int z) {
			return x >= lo.x && x <= hi.x && y >= lo.y && y <= hi.y && z >= lo.z && z <= hi.z;
		};
		if (!collectShape(lo, hi, hollow, inside, positions, error)) {
			return ctx.result(id, error, true);
		}
	} else if (shape == "sphere" || shape == "ellipsoid") {
		glm::vec3 center(0.0f);
		glm::vec3 radii(1.0f);
		glm::ivec3 lo(0);
		glm::ivec3 hi(0);
		float limit = 1.0001f;
		if (shape == "sphere") {
			if (!args.contains("center") || !args.contains("radius")) {
				return ctx.result(id, "sphere requires center and radius", true);
			}
			glm::ivec3 cell(0);
			if (!parseCell(args.get("center"), cell, "center", error)) {
				return ctx.result(id, error, true);
			}
			const float radius = (float)args.get("radius").doubleVal();
			if (radius < 0.0f) {
				return ctx.result(id, "radius must be >= 0", true);
			}
			center = glm::vec3(cell) + 0.5f;
			const float radiusVoxels = core_max(0.5f, radius + 0.5f);
			radii = glm::vec3(radiusVoxels);
			lo = glm::ivec3(math::floorToInt(center.x - radii.x), math::floorToInt(center.y - radii.y),
							math::floorToInt(center.z - radii.z));
			hi = glm::ivec3(math::floorToInt(center.x + radii.x), math::floorToInt(center.y + radii.y),
							math::floorToInt(center.z + radii.z));
			limit = 0.9999f;
		} else {
			if (!readCorners(args, lo, hi, error)) {
				return ctx.result(id, error, true);
			}
			center = (glm::vec3(lo) + glm::vec3(hi) + glm::vec3(1.0f)) * 0.5f;
			radii = (glm::vec3(hi) - glm::vec3(lo) + glm::vec3(1.0f)) * 0.5f;
		}
		auto inside = [center, radii, limit](int x, int y, int z) {
			const glm::vec3 delta((x + 0.5f - center.x) / core_max(radii.x, 0.01f),
								  (y + 0.5f - center.y) / core_max(radii.y, 0.01f),
								  (z + 0.5f - center.z) / core_max(radii.z, 0.01f));
			return glm::dot(delta, delta) < limit;
		};
		if (!collectShape(lo, hi, hollow, inside, positions, error)) {
			return ctx.result(id, error, true);
		}
	} else if (shape == "cylinder") {
		if (!args.contains("base") || !args.contains("radius")) {
			return ctx.result(id, "cylinder requires base and radius", true);
		}
		glm::ivec3 base(0);
		if (!parseCell(args.get("base"), base, "base", error)) {
			return ctx.result(id, error, true);
		}
		const float radius = (float)args.get("radius").doubleVal();
		if (radius < 0.0f) {
			return ctx.result(id, "radius must be >= 0", true);
		}
		const int height = core_max(1, args.intVal("height", 1));
		math::Axis axis = math::Axis::Y;
		if (args.contains("axis") && !parseAxis(args.strVal("axis", "y"), axis)) {
			return ctx.result(id, "axis must be x, y or z", true);
		}
		const int extent = math::ceilToInt(radius + 0.5f);
		const float limit = (radius + 0.5f) * (radius + 0.5f) - 0.01f;
		glm::ivec3 lo = base;
		glm::ivec3 hi = base;
		setAxisComponent(hi, axis, axisComponent(base, axis) + height - 1);
		const math::Axis uAxis = axis == math::Axis::X ? math::Axis::Y : math::Axis::X;
		const math::Axis vAxis = axis == math::Axis::Z ? math::Axis::Y : math::Axis::Z;
		setAxisComponent(lo, uAxis, axisComponent(base, uAxis) - extent);
		setAxisComponent(hi, uAxis, axisComponent(base, uAxis) + extent);
		setAxisComponent(lo, vAxis, axisComponent(base, vAxis) - extent);
		setAxisComponent(hi, vAxis, axisComponent(base, vAxis) + extent);
		auto inside = [base, height, axis, limit](int x, int y, int z) {
			const glm::ivec3 pos(x, y, z);
			const int along = axisComponent(pos, axis) - axisComponent(base, axis);
			if (along < 0 || along >= height) {
				return false;
			}
			int u = 0;
			int v = 0;
			if (axis == math::Axis::X) {
				u = y - base.y;
				v = z - base.z;
			} else if (axis == math::Axis::Z) {
				u = x - base.x;
				v = y - base.y;
			} else {
				u = x - base.x;
				v = z - base.z;
			}
			return (float)(u * u + v * v) < limit;
		};
		if (!collectShape(lo, hi, hollow, inside, positions, error)) {
			return ctx.result(id, error, true);
		}
	} else if (shape == "line") {
		if (!args.contains("from") || !args.contains("to")) {
			return ctx.result(id, "line requires from and to", true);
		}
		glm::ivec3 from(0);
		glm::ivec3 to(0);
		if (!parseCell(args.get("from"), from, "from", error) || !parseCell(args.get("to"), to, "to", error)) {
			return ctx.result(id, error, true);
		}
		const glm::ivec3 delta = to - from;
		const int ax = delta.x < 0 ? -delta.x : delta.x;
		const int ay = delta.y < 0 ? -delta.y : delta.y;
		const int az = delta.z < 0 ? -delta.z : delta.z;
		const int steps = core_max(ax, core_max(ay, az));
		if (steps > MaxShapeCells) {
			return ctx.result(id, "That line is too long", true);
		}
		for (int i = 0; i <= steps; ++i) {
			const float t = steps == 0 ? 0.0f : (float)i / (float)steps;
			const glm::ivec3 pos((int)glm::round((float)from.x + (float)delta.x * t),
								 (int)glm::round((float)from.y + (float)delta.y * t),
								 (int)glm::round((float)from.z + (float)delta.z * t));
			positions.push_back(pos);
		}
	} else {
		return ctx.result(id, "shape must be box, sphere, ellipsoid, cylinder or line", true);
	}

	core::DynamicArray<PaintCell> cells;
	cells.reserve(positions.size());
	for (const glm::ivec3 &pos : positions) {
		PaintCell cell;
		cell.pos = pos;
		cell.erase = color.erase;
		cell.index = color.index;
		cells.push_back(cell);
	}
	int changed = 0;
	int skipped = 0;
	if (!paintCells(ctx, *node, cells, changed, skipped)) {
		return ctx.result(id, "Volume not found - this is no model node", true);
	}
	core::String message = core::String::format("%s of %i cells, %i voxels changed, %i outside the volume.", shape.c_str(),
												(int)positions.size(), changed, skipped);
	if (color.approximated) {
		message += " The palette was full, so a closest existing color was used.";
	}
	return ctx.result(id, message, false);
}

} // namespace voxedit
