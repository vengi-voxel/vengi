/**
 * @file
 */

#pragma once

#include "app/Async.h"
#include "color/ColorUtil.h"
#include "core/GLM.h"
#include "core/Trace.h"
#include "core/collection/DynamicSet.h"
#include "core/concurrent/Atomic.h"
#include "palette/Palette.h"
#include "voxel/Connectivity.h"
#include "voxel/Face.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include <stdint.h>

namespace voxelutil {

enum class VisitorOrder : uint8_t {
	XYZ,
	ZYX,
	ZXY,
	XmZY,
	mXZY,
	mXmZY,
	mXZmY,
	XmZmY,
	mXmZmY,
	XZY,
	XZmY,
	YXZ,
	YZX,
	YmXZ,
	mYZX,
	YZmX,
	mYmXZ,
	mYXmZ,
	mYmZmX,
	mYmXmZ,
	mZmXmY,
	ZmXmY,
	ZmXY,
	YXmZ,
	ZXmY,
	mZXY,
	mYZmX,
	mYXZ,
	mZXmY,
	mZmXY,
	Max
};
static const char *VisitorOrderStr[] = {
	"XYZ",
	"ZYX",
	"ZXY",
	"XmZY",
	"mXZY",
	"mXmZY",
	"mXZmY",
	"XmZmY",
	"mXmZmY",
	"XZY",
	"XZmY",
	"YXZ",
	"YZX",
	"YmXZ",
	"mYZX",
	"YZmX",
	"mYmXZ",
	"mYXmZ",
	"mYmZmX",
	"mYmXmZ",
	"mZmXmY",
	"ZmXmY",
	"ZmXY",
	"YXmZ",
	"ZXmY",
	"mZXY",
	"mYZmX",
	"mYXZ",
	"mZXmY",
	"mZmXY"
};
static_assert(lengthof(VisitorOrderStr) == (int)VisitorOrder::Max, "Array size mismatch");

/**
 * @brief Will skip air voxels on volume
 * @note Visitor condition
 */
struct VisitSolid {
	template<class Sampler>
	inline bool operator()(const Sampler &sampler) const {
		return !isAir(sampler.voxel().getMaterial());
	}
};

/**
 * @note Visitor condition
 */
struct VisitInvisible {
	template<class Sampler>
	inline bool operator()(const Sampler &sampler) const {
		if (isAir(sampler.voxel().getMaterial())) {
			return false;
		}
		return visibleFaces(sampler) == voxel::FaceBits::None;
	}
};

/**
 * @note Visitor condition
 */
struct VisitVisible {
	template<class Sampler>
	inline bool operator()(const Sampler &sampler) const {
		if (isAir(sampler.voxel().getMaterial())) {
			return false;
		}
		return visibleFaces(sampler) != voxel::FaceBits::None;
	}
};

/**
 * @note Visitor condition
 */
struct VisitAll {
	template<class Sampler>
	inline bool operator()(const Sampler &) const {
		return true;
	}
};

/**
 * @note Visitor condition
 */
struct VisitEmpty {
	template<class Sampler>
	inline bool operator()(const Sampler &sampler) const {
		return isAir(sampler.voxel().getMaterial());
	}
};

/**
 * @note Visitor condition for the same voxel color - while skipping air voxels - so the voxel type is ignored here
 * @sa VisitVoxelType()
 */
struct VisitVoxelColor {
	const uint8_t _color;
	VisitVoxelColor(uint8_t color) : _color(color) {
	}
	VisitVoxelColor(voxel::Voxel voxel) : _color(voxel.getColor()) {
	}
	template<class Sampler>
	inline bool operator()(const Sampler &sampler) const {
		if (sampler.voxel().getMaterial() == voxel::VoxelType::Air) {
			return false;
		}
		return sampler.voxel().getColor() == _color;
	}
};

/**
 * @note Visitor condition for the same voxel type - does not check the color
 * @sa VisitVoxelColor()
 */
struct VisitVoxelType {
	const voxel::VoxelType _type;
	VisitVoxelType(voxel::VoxelType type) : _type(type) {
	}
	VisitVoxelType(voxel::Voxel voxel) : _type(voxel.getMaterial()) {
	}
	template<class Sampler>
	inline bool operator()(const Sampler &sampler) const {
		return sampler.voxel().getMaterial() == _type;
	}
};

struct VisitVoxelFuzzyColor {
	const palette::Palette &_palette;
	const uint8_t _colorIndex;
	const float _threshold;

	VisitVoxelFuzzyColor(const palette::Palette &palette, uint8_t colorIndex, float threshold)
		: _palette(palette), _colorIndex(colorIndex), _threshold(threshold) {
	}

	template<class Sampler>
	inline bool operator()(const Sampler &sampler) const {
		if (sampler.voxel().getMaterial() == voxel::VoxelType::Air) {
			return false;
		}
		const uint8_t voxelColorIndex = sampler.voxel().getColor();
		if (voxelColorIndex == _colorIndex) {
			return true;
		}
		const color::RGBA c1 = _palette.color(_colorIndex);
		const color::RGBA c2 = _palette.color(voxelColorIndex);
		return color::getDistance(c1, c2, color::Distance::Approximation) < _threshold;
	}
};

/**
 * @note Visitor
 */
struct EmptyVisitor {
	inline void operator()(int x, int y, int z, const voxel::Voxel &voxel) const {
	}
};

// the visitor can return a boolean value to indicate whether the inner loop should break
#define VISITOR_INNER_PART                                                                                             \
	if constexpr (std::is_same_v<decltype(visitor(x, y, z, voxel)), bool>) {                                          \
		const bool breakInnerLoop = visitor(x, y, z, voxel);                                                           \
		++cnt;                                                                                                         \
		if (breakInnerLoop) {                                                                                          \
			break;                                                                                                     \
		}                                                                                                              \
	} else {                                                                                                           \
		visitor(x, y, z, voxel);                                                                                       \
		++cnt;                                                                                                         \
	}

template<class Sampler, class Visitor = EmptyVisitor, typename Condition = VisitSolid>
int visitSampler(Sampler &sampler, const voxel::Region &region, int xOff, int yOff, int zOff,
				 Visitor &&visitor = Visitor(), Condition condition = Condition(),
				 VisitorOrder order = VisitorOrder::ZYX) {
	core_trace_scoped(VisitVolume);
	int cnt = 0;

	switch (order) {
	case VisitorOrder::XYZ:
		sampler.setPosition(region.getLowerCorner());
		for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
			Sampler sampler2 = sampler;
			for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
				Sampler sampler3 = sampler2;
				for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.movePositiveZ(zOff);
						continue;
					}
					sampler3.movePositiveZ(zOff);
					VISITOR_INNER_PART
				}
				sampler2.movePositiveY(yOff);
			}
			sampler.movePositiveX(xOff);
		}
		break;
	case VisitorOrder::ZYX:
		sampler.setPosition(region.getLowerCorner());
		for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
			Sampler sampler2 = sampler;
			for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
				Sampler sampler3 = sampler2;
				for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.movePositiveX(xOff);
						continue;
					}
					sampler3.movePositiveX(xOff);
					VISITOR_INNER_PART
				}
				sampler2.movePositiveY(yOff);
			}
			sampler.movePositiveZ(zOff);
		}
		break;
	case VisitorOrder::ZXY:
		sampler.setPosition(region.getLowerCorner());
		for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
			Sampler sampler2 = sampler;
			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
				Sampler sampler3 = sampler2;
				for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.movePositiveY(yOff);
						continue;
					}
					sampler3.movePositiveY(yOff);
					VISITOR_INNER_PART
				}
				sampler2.movePositiveX(xOff);
			}
			sampler.movePositiveZ(zOff);
		}
		break;
	case VisitorOrder::XZY:
		sampler.setPosition(region.getLowerCorner());
		for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
			Sampler sampler2 = sampler;
			for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
				Sampler sampler3 = sampler2;
				for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.movePositiveY(yOff);
						continue;
					}
					sampler3.movePositiveY(yOff);
					VISITOR_INNER_PART
				}
				sampler2.movePositiveZ(zOff);
			}
			sampler.movePositiveX(xOff);
		}
		break;
	case VisitorOrder::XZmY:
		sampler.setPosition(region.getLowerX(), region.getUpperY(), region.getLowerZ());
		for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
			Sampler sampler2 = sampler;
			for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
				Sampler sampler3 = sampler2;
				for (int32_t y = region.getUpperY(); y >= region.getLowerY(); y -= yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.moveNegativeY(yOff);
						continue;
					}
					sampler3.moveNegativeY(yOff);
					VISITOR_INNER_PART
				}
				sampler2.movePositiveZ(zOff);
			}
			sampler.movePositiveX(xOff);
		}
		break;
	case VisitorOrder::mXmZY:
		sampler.setPosition(region.getUpperX(), region.getLowerY(), region.getUpperZ());
		for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
			Sampler sampler2 = sampler;
			for (int32_t z = region.getUpperZ(); z >= region.getLowerZ(); z -= zOff) {
				Sampler sampler3 = sampler2;
				for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.movePositiveY(yOff);
						continue;
					}
					sampler3.movePositiveY(yOff);
					VISITOR_INNER_PART
				}
				sampler2.moveNegativeZ(zOff);
			}
			sampler.moveNegativeX(xOff);
		}
		break;
	case VisitorOrder::mXZY:
		sampler.setPosition(region.getUpperX(), region.getLowerY(), region.getLowerZ());
		for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
			Sampler sampler2 = sampler;
			for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
				Sampler sampler3 = sampler2;
				for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.movePositiveY(yOff);
						continue;
					}
					sampler3.movePositiveY(yOff);
					VISITOR_INNER_PART
				}
				sampler2.movePositiveZ(zOff);
			}
			sampler.moveNegativeX(xOff);
		}
		break;
	case VisitorOrder::XmZY:
		sampler.setPosition(region.getLowerX(), region.getLowerY(), region.getUpperZ());
		for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
			Sampler sampler2 = sampler;
			for (int32_t z = region.getUpperZ(); z >= region.getLowerZ(); z -= zOff) {
				Sampler sampler3 = sampler2;
				for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.movePositiveY(yOff);
						continue;
					}
					sampler3.movePositiveY(yOff);
					VISITOR_INNER_PART
				}
				sampler2.moveNegativeZ(zOff);
			}
			sampler.movePositiveX(xOff);
		}
		break;
	case VisitorOrder::XmZmY:
		sampler.setPosition(region.getLowerX(), region.getUpperY(), region.getUpperZ());
		for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
			Sampler sampler2 = sampler;
			for (int32_t z = region.getUpperZ(); z >= region.getLowerZ(); z -= zOff) {
				Sampler sampler3 = sampler2;
				for (int32_t y = region.getUpperY(); y >= region.getLowerY(); y -= yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.moveNegativeY(yOff);
						continue;
					}
					sampler3.moveNegativeY(yOff);
					VISITOR_INNER_PART
				}
				sampler2.moveNegativeZ(zOff);
			}
			sampler.movePositiveX(xOff);
		}
		break;
	case VisitorOrder::mXmZmY:
		sampler.setPosition(region.getUpperX(), region.getUpperY(), region.getUpperZ());
		for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
			Sampler sampler2 = sampler;
			for (int32_t z = region.getUpperZ(); z >= region.getLowerZ(); z -= zOff) {
				Sampler sampler3 = sampler2;
				for (int32_t y = region.getUpperY(); y >= region.getLowerY(); y -= yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.moveNegativeY(yOff);
						continue;
					}
					sampler3.moveNegativeY(yOff);
					VISITOR_INNER_PART
				}
				sampler2.moveNegativeZ(zOff);
			}
			sampler.moveNegativeX(xOff);
		}
		break;
	case VisitorOrder::mXZmY:
		sampler.setPosition(region.getUpperX(), region.getUpperY(), region.getLowerZ());
		for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
			Sampler sampler2 = sampler;
			for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
				Sampler sampler3 = sampler2;
				for (int32_t y = region.getUpperY(); y >= region.getLowerY(); y -= yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.moveNegativeY(yOff);
						continue;
					}
					sampler3.moveNegativeY(yOff);
					VISITOR_INNER_PART
				}
				sampler2.movePositiveZ(zOff);
			}
			sampler.moveNegativeX(xOff);
		}
		break;
	case VisitorOrder::YXZ:
		sampler.setPosition(region.getLowerCorner());
		for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
			Sampler sampler2 = sampler;
			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
				Sampler sampler3 = sampler2;
				for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.movePositiveZ(zOff);
						continue;
					}
					sampler3.movePositiveZ(zOff);
					VISITOR_INNER_PART
				}
				sampler2.movePositiveX(xOff);
			}
			sampler.movePositiveY(yOff);
		}
		break;
	case VisitorOrder::mZmXY:
		sampler.setPosition(region.getUpperX(), region.getLowerY(), region.getUpperZ());
		for (int32_t z = region.getUpperZ(); z >= region.getLowerZ(); z -= zOff) {
			Sampler sampler2 = sampler;
			for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
				Sampler sampler3 = sampler2;
				for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.movePositiveY(yOff);
						continue;
					}
					sampler3.movePositiveY(yOff);
					VISITOR_INNER_PART
				}
				sampler2.moveNegativeX(xOff);
			}
			sampler.moveNegativeZ(zOff);
		}
		break;
	case VisitorOrder::mZXmY:
		sampler.setPosition(region.getLowerX(), region.getUpperY(), region.getUpperZ());
		for (int32_t z = region.getUpperZ(); z >= region.getLowerZ(); z -= zOff) {
			Sampler sampler2 = sampler;
			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
				Sampler sampler3 = sampler2;
				for (int32_t y = region.getUpperY(); y >= region.getLowerY(); y -= yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.moveNegativeY(yOff);
						continue;
					}
					sampler3.moveNegativeY(yOff);
					VISITOR_INNER_PART
				}
				sampler2.movePositiveX(xOff);
			}
			sampler.moveNegativeZ(zOff);
		}
		break;
	case VisitorOrder::mYXZ:
		sampler.setPosition(region.getLowerX(), region.getUpperY(), region.getLowerZ());
		for (int32_t y = region.getUpperY(); y >= region.getLowerY(); y -= yOff) {
			Sampler sampler2 = sampler;
			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
				Sampler sampler3 = sampler2;
				for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.movePositiveZ(zOff);
						continue;
					}
					sampler3.movePositiveZ(zOff);
					VISITOR_INNER_PART
				}
				sampler2.movePositiveX(xOff);
			}
			sampler.moveNegativeY(yOff);
		}
		break;
	case VisitorOrder::YZX:
		sampler.setPosition(region.getLowerCorner());
		for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
			Sampler sampler2 = sampler;
			for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
				Sampler sampler3 = sampler2;
				for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.movePositiveX(xOff);
						continue;
					}
					sampler3.movePositiveX(xOff);
					VISITOR_INNER_PART
				}
				sampler2.movePositiveZ(zOff);
			}
			sampler.movePositiveY(yOff);
		}
		break;
	case VisitorOrder::mYZmX:
		sampler.setPosition(region.getUpperX(), region.getUpperY(), region.getLowerZ());
		for (int32_t y = region.getUpperY(); y >= region.getLowerY(); y -= yOff) {
			Sampler sampler2 = sampler;
			for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
				Sampler sampler3 = sampler2;
				for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.moveNegativeX(xOff);
						continue;
					}
					sampler3.moveNegativeX(xOff);
					VISITOR_INNER_PART
				}
				sampler2.movePositiveZ(zOff);
			}
			sampler.moveNegativeY(yOff);
		}
		break;
	case VisitorOrder::YZmX:
		sampler.setPosition(region.getUpperX(), region.getLowerY(), region.getLowerZ());
		for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
			Sampler sampler2 = sampler;
			for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
				Sampler sampler3 = sampler2;
				for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.moveNegativeX(xOff);
						continue;
					}
					sampler3.moveNegativeX(xOff);
					VISITOR_INNER_PART
				}
				sampler2.movePositiveZ(zOff);
			}
			sampler.movePositiveY(yOff);
		}
		break;
	case VisitorOrder::mYZX:
		sampler.setPosition(region.getLowerX(), region.getUpperY(), region.getLowerZ());
		for (int32_t y = region.getUpperY(); y >= region.getLowerY(); y -= yOff) {
			Sampler sampler2 = sampler;
			for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
				Sampler sampler3 = sampler2;
				for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.movePositiveX(xOff);
						continue;
					}
					sampler3.movePositiveX(xOff);
					VISITOR_INNER_PART
				}
				sampler2.movePositiveZ(zOff);
			}
			sampler.moveNegativeY(yOff);
		}
		break;
	case VisitorOrder::mYmXZ:
		sampler.setPosition(region.getUpperX(), region.getUpperY(), region.getLowerZ());
		for (int32_t y = region.getUpperY(); y >= region.getLowerY(); y -= yOff) {
			Sampler sampler2 = sampler;
			for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
				Sampler sampler3 = sampler2;
				for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.movePositiveZ(zOff);
						continue;
					}
					sampler3.movePositiveZ(zOff);
					VISITOR_INNER_PART
				}
				sampler2.moveNegativeX(xOff);
			}
			sampler.moveNegativeY(yOff);
		}
		break;
	case VisitorOrder::mYXmZ:
		sampler.setPosition(region.getLowerX(), region.getUpperY(), region.getUpperZ());
		for (int32_t y = region.getUpperY(); y >= region.getLowerY(); y -= yOff) {
			Sampler sampler2 = sampler;
			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
				Sampler sampler3 = sampler2;
				for (int32_t z = region.getUpperZ(); z >= region.getLowerZ(); z -= zOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.moveNegativeZ(zOff);
						continue;
					}
					sampler3.moveNegativeZ(zOff);
					VISITOR_INNER_PART
				}
				sampler2.movePositiveX(xOff);
			}
			sampler.moveNegativeY(yOff);
		}
		break;
	case VisitorOrder::mYmXmZ:
		sampler.setPosition(region.getUpperX(), region.getUpperY(), region.getUpperZ());
		for (int32_t y = region.getUpperY(); y >= region.getLowerY(); y -= yOff) {
			Sampler sampler2 = sampler;
			for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
				Sampler sampler3 = sampler2;
				for (int32_t z = region.getUpperZ(); z >= region.getLowerZ(); z -= zOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.moveNegativeZ(zOff);
						continue;
					}
					sampler3.moveNegativeZ(xOff);
					VISITOR_INNER_PART
				}
				sampler2.moveNegativeX(zOff);
			}
			sampler.moveNegativeY(yOff);
		}
		break;
	case VisitorOrder::mYmZmX:
		sampler.setPosition(region.getUpperX(), region.getUpperY(), region.getUpperZ());
		for (int32_t y = region.getUpperY(); y >= region.getLowerY(); y -= yOff) {
			Sampler sampler2 = sampler;
			for (int32_t z = region.getUpperZ(); z >= region.getLowerZ(); z -= zOff) {
				Sampler sampler3 = sampler2;
				for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.moveNegativeX(xOff);
						continue;
					}
					sampler3.moveNegativeX(xOff);
					VISITOR_INNER_PART
				}
				sampler2.moveNegativeZ(zOff);
			}
			sampler.moveNegativeY(yOff);
		}
		break;
	case VisitorOrder::mZmXmY:
		sampler.setPosition(region.getUpperX(), region.getUpperY(), region.getUpperZ());
		for (int32_t z = region.getUpperZ(); z >= region.getLowerZ(); z -= zOff) {
			Sampler sampler2 = sampler;
			for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
				Sampler sampler3 = sampler2;
				for (int32_t y = region.getUpperY(); y >= region.getLowerY(); y -= yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.moveNegativeY(yOff);
						continue;
					}
					sampler3.moveNegativeY(yOff);
					VISITOR_INNER_PART
				}
				sampler2.moveNegativeX(xOff);
			}
			sampler.moveNegativeZ(zOff);
		}
		break;
	case VisitorOrder::ZmXmY:
		sampler.setPosition(region.getUpperX(), region.getUpperY(), region.getLowerZ());
		for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
			Sampler sampler2 = sampler;
			for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
				Sampler sampler3 = sampler2;
				for (int32_t y = region.getUpperY(); y >= region.getLowerY(); y -= yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.moveNegativeY(yOff);
						continue;
					}
					sampler3.moveNegativeY(yOff);
					VISITOR_INNER_PART
				}
				sampler2.moveNegativeX(xOff);
			}
			sampler.movePositiveZ(zOff);
		}
		break;
	case VisitorOrder::YmXZ:
		sampler.setPosition(region.getUpperX(), region.getLowerY(), region.getLowerZ());
		for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
			Sampler sampler2 = sampler;
			for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
				Sampler sampler3 = sampler2;
				for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.movePositiveZ(zOff);
						continue;
					}
					sampler3.movePositiveZ(zOff);
					VISITOR_INNER_PART
				}
				sampler2.moveNegativeX(xOff);
			}
			sampler.movePositiveY(yOff);
		}
		break;
	case VisitorOrder::ZmXY:
		sampler.setPosition(region.getUpperX(), region.getLowerY(), region.getLowerZ());
		for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
			Sampler sampler2 = sampler;
			for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
				Sampler sampler3 = sampler2;
				for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.movePositiveY(yOff);
						continue;
					}
					sampler3.movePositiveY(yOff);
					VISITOR_INNER_PART
				}
				sampler2.moveNegativeX(xOff);
			}
			sampler.movePositiveZ(zOff);
		}
		break;
	case VisitorOrder::YXmZ:
		sampler.setPosition(region.getLowerX(), region.getLowerY(), region.getUpperZ());
		for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
			Sampler sampler2 = sampler;
			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
				Sampler sampler3 = sampler2;
				for (int32_t z = region.getUpperZ(); z >= region.getLowerZ(); z -= zOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.moveNegativeZ(zOff);
						continue;
					}
					sampler3.moveNegativeZ(zOff);
					VISITOR_INNER_PART
				}
				sampler2.movePositiveX(xOff);
			}
			sampler.movePositiveY(yOff);
		}
		break;
	case VisitorOrder::mZXY:
		sampler.setPosition(region.getLowerX(), region.getLowerY(), region.getUpperZ());
		for (int32_t z = region.getUpperZ(); z >= region.getLowerZ(); z -= zOff) {
			Sampler sampler2 = sampler;
			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
				Sampler sampler3 = sampler2;
				for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.movePositiveY(yOff);
						continue;
					}
					sampler3.movePositiveY(yOff);
					VISITOR_INNER_PART
				}
				sampler2.movePositiveX(xOff);
			}
			sampler.moveNegativeZ(zOff);
		}
		break;
	case VisitorOrder::ZXmY:
		sampler.setPosition(region.getLowerX(), region.getUpperY(), region.getLowerZ());
		for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
			Sampler sampler2 = sampler;
			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
				Sampler sampler3 = sampler2;
				for (int32_t y = region.getUpperY(); y >= region.getLowerY(); y -= yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
					if (!condition(sampler3)) {
						sampler3.moveNegativeY(yOff);
						continue;
					}
					sampler3.moveNegativeY(yOff);
					VISITOR_INNER_PART
				}
				sampler2.movePositiveX(xOff);
			}
			sampler.movePositiveZ(zOff);
		}
		break;
	case VisitorOrder::Max:
		break;
	}

#undef LOOP
	return cnt;
}

template<class Volume, class Visitor = EmptyVisitor, typename Condition = VisitSolid>
int visitVolume(const Volume &volume, const voxel::Region &region, int xOff, int yOff, int zOff, Visitor &&visitor = Visitor(),
				Condition condition = Condition(), VisitorOrder order = VisitorOrder::ZYX) {
	typename Volume::Sampler sampler(volume);
	return visitSampler(sampler, region, xOff, yOff, zOff, visitor, condition, order);
}

template<class Volume, class Visitor = EmptyVisitor, typename Condition = VisitSolid>
int visitVolume(const Volume &volume, int xOff, int yOff, int zOff, Visitor &&visitor = Visitor(),
				Condition condition = Condition(), VisitorOrder order = VisitorOrder::ZYX) {
	const voxel::Region &region = volume.region();
	return visitVolume(volume, region, xOff, yOff, zOff, visitor, condition, order);
}

template<class Volume, class Visitor = EmptyVisitor, typename Condition = VisitSolid>
int visitVolume(const Volume &volume, Visitor &&visitor = Visitor(), Condition condition = Condition(),
				VisitorOrder order = VisitorOrder::ZYX) {
	return visitVolume(volume, 1, 1, 1, visitor, condition, order);
}

template<class Volume, class Visitor = EmptyVisitor, typename Condition = VisitSolid>
int visitVolume(const Volume &volume, const voxel::Region &region, Visitor &&visitor = Visitor(), Condition condition = Condition(),
				VisitorOrder order = VisitorOrder::ZYX) {
	return visitVolume(volume, region, 1, 1, 1, visitor, condition, order);
}

/**
 * @brief Visit all non-visible voxels - voxels that are surrounded by solid voxels on all sides
 * @sa visibleFaces()
 * @sa visitSurfaceVolume()
 */
template<class Volume, class Visitor = EmptyVisitor>
int visitInvisibleVolume(const Volume &volume, Visitor &&visitor = Visitor(), VisitorOrder order = VisitorOrder::ZYX) {
	return visitVolume(volume, visitor, VisitInvisible(), order);
}

/**
 * @brief Visit only surface voxels - those voxels have at least one visible face (or an air voxel next to it)
 * @sa visitInvisibleVolume()
 */
template<class Volume, class Visitor = EmptyVisitor>
int visitSurfaceVolume(const Volume &volume, Visitor &&visitor = Visitor(), VisitorOrder order = VisitorOrder::ZYX) {
	return visitVolume(volume, visitor, VisitVisible(), order);
}

/**
 * @brief Visit only surface voxels - those voxels have at least one visible face (or an air voxel next to it)
 * @sa visitInvisibleVolume()
 */
template<class Volume, class Visitor = EmptyVisitor>
int visitSurfaceVolumeParallel(const Volume &volume, Visitor &&visitor = Visitor(), VisitorOrder order = VisitorOrder::ZYX) {
	return visitVolumeParallel(volume, visitor, VisitVisible(), order);
}

/**
 * @return The number of voxels visited.
 */
template<class Volume, class Visitor = EmptyVisitor>
int visitFace(const Volume &volume, const voxel::Region &region, voxel::FaceNames face, Visitor &&visitor = Visitor(),
			  VisitorOrder order = VisitorOrder::ZYX, bool searchSurface = false) {
	typename Volume::Sampler sampler(volume);
	voxel::FaceBits faceBits = voxel::faceBits(face);
	constexpr bool skipEmpty = true;

	auto visitorInternal = [&] (int x, int y, int z, const voxel::Voxel &voxel) {
		if (!searchSurface) {
			visitor(x, y, z, voxel);
			return true;
		}
		typename Volume::Sampler sampler2(volume);
		sampler2.setPosition(x, y, z);
		if ((visibleFaces(sampler2, skipEmpty) & faceBits) != voxel::FaceBits::None) {
			visitor(x, y, z, voxel);
			return true;
		}
		return false;
	};
	return visitVolume(volume, region, 1, 1, 1, visitorInternal, VisitAll(), order);
}

/**
 * @return The number of voxels visited.
 */
template<class Volume, class Visitor = EmptyVisitor>
int visitFace(const Volume &volume, const voxel::Region &region, voxel::FaceNames face, Visitor &&visitor = Visitor(),
			  bool searchSurface = false) {
	// only the last axis matters here
	VisitorOrder visitorOrder;
	switch (face) {
	case voxel::FaceNames::Front:
		visitorOrder = VisitorOrder::mYmXZ;
		break;
	case voxel::FaceNames::Back:
		visitorOrder = VisitorOrder::mYXmZ;
		break;
	case voxel::FaceNames::Right:
		visitorOrder = VisitorOrder::mYmZmX;
		break;
	case voxel::FaceNames::Left:
		visitorOrder = VisitorOrder::mYZX;
		break;
	case voxel::FaceNames::Up:
		visitorOrder = VisitorOrder::mZmXmY;
		break;
	case voxel::FaceNames::Down:
		visitorOrder = VisitorOrder::ZmXY;
		break;
	default:
		return 0;
	}
	return visitFace(volume, region, face, visitor, visitorOrder, searchSurface);
}

template<class Volume, class Visitor = EmptyVisitor>
int visitFace(const Volume &volume, voxel::FaceNames face, Visitor &&visitor = Visitor(), VisitorOrder order = VisitorOrder::Max, bool searchSurface = false) {
	const voxel::Region &region = volume.region();
	if (order == VisitorOrder::Max) {
		return visitFace(volume, region, face, visitor, searchSurface);
	}
	return visitFace(volume, region, face, visitor, order, searchSurface);
}

template<class Volume, class Visitor = EmptyVisitor, typename Condition = VisitSolid>
int visitVolumeParallel(const Volume &volume, const voxel::Region &region, Visitor &&visitor = Visitor(),
						Condition condition = Condition(), VisitorOrder order = VisitorOrder::ZYX) {
	core_trace_scoped(VisitVolumeParallel);
	core::AtomicInt cnt(0);

	switch (order) {
	case VisitorOrder::XYZ:
	case VisitorOrder::XZmY:
	case VisitorOrder::XmZmY:
	case VisitorOrder::XZY:
	case VisitorOrder::XmZY:
	case VisitorOrder::mXZY:
	case VisitorOrder::mXmZY:
	case VisitorOrder::mXZmY:
	case VisitorOrder::mXmZmY:
		// anything were the outer loop is x is handled here
		app::for_parallel(
			region.getLowerX(), region.getUpperX() + 1,
			[&](int32_t start, int32_t end) {
				// we have to subtract 1 because the end is exclusive
				// the first axis from the visitor order is the outer loop and has be to handled in the subRegion
				const voxel::Region subRegion(start, region.getLowerY(), region.getLowerZ(), end - 1,
											  region.getUpperY(), region.getUpperZ());
				cnt.increment(visitVolume(volume, subRegion, visitor, condition, order));
			},
			true);
		break;

	case VisitorOrder::YXZ:
	case VisitorOrder::YZX:
	case VisitorOrder::YmXZ:
	case VisitorOrder::YZmX:
	case VisitorOrder::YXmZ:
	case VisitorOrder::mYZX:
	case VisitorOrder::mYmXZ:
	case VisitorOrder::mYXmZ:
	case VisitorOrder::mYmZmX:
	case VisitorOrder::mYmXmZ:
	case VisitorOrder::mYZmX:
	case VisitorOrder::mYXZ:
		// anything were the outer loop is y is handled here
		app::for_parallel(
			region.getLowerY(), region.getUpperY() + 1,
			[&](int32_t start, int32_t end) {
				// we have to subtract 1 because the end is exclusive
				// the first axis from the visitor order is the outer loop and has be to handled in the subRegion
				const voxel::Region subRegion(region.getLowerX(), start, region.getLowerZ(), region.getUpperX(),
											  end - 1, region.getUpperZ());
				cnt.increment(visitVolume(volume, subRegion, visitor, condition, order));
			},
			true);
		break;
	case VisitorOrder::ZYX:
	case VisitorOrder::ZXY:
	case VisitorOrder::ZXmY:
	case VisitorOrder::ZmXmY:
	case VisitorOrder::ZmXY:
	case VisitorOrder::mZXY:
	case VisitorOrder::mZXmY:
	case VisitorOrder::mZmXY:
	case VisitorOrder::mZmXmY:
		// anything were the outer loop is z is handled here
		app::for_parallel(
			region.getLowerZ(), region.getUpperZ() + 1,
			[&](int32_t start, int32_t end) {
				// we have to subtract 1 because the end is exclusive
				// the first axis from the visitor order is the outer loop and has be to handled in the subRegion
				const voxel::Region subRegion(region.getLowerX(), region.getLowerY(), start, region.getUpperX(),
											  region.getUpperY(), end - 1);
				cnt.increment(visitVolume(volume, subRegion, visitor, condition, order));
			},
			true);
		break;
	case voxelutil::VisitorOrder::Max:
		break;
	}
	return cnt;
}

template<class Volume, class Visitor = EmptyVisitor, typename Condition = VisitSolid>
int visitVolumeParallel(const Volume &volume, Visitor &&visitor = Visitor(), Condition condition = Condition(),
						VisitorOrder order = VisitorOrder::ZYX) {
	const voxel::Region &region = volume.region();
	return visitVolumeParallel(volume, region, visitor, condition, order);
}

template<class Volume>
inline int countVoxels(const Volume &volume) {
	return voxelutil::visitVolumeParallel(volume);
}

template<class Volume>
int countVoxelsByType(const Volume &volume, const voxel::Voxel &voxel) {
	return voxelutil::visitVolumeParallel(volume, voxelutil::EmptyVisitor(), voxelutil::VisitVoxelType(voxel.getMaterial()));
}

template<class Volume>
int countVoxelsByColor(const Volume &volume, const voxel::Voxel &voxel) {
	return voxelutil::visitVolumeParallel(volume, voxelutil::EmptyVisitor(), voxelutil::VisitVoxelColor(voxel.getColor()));
}

typedef core::DynamicSet<glm::ivec3, 1031, glm::hash<glm::ivec3>> VisitedSet;

template<class Volume, class Visitor, class Condition>
static int visitConnectedByVoxel_r(const Volume &volume, const voxel::Voxel &voxel, const glm::ivec3 &position, Visitor &visitor, Condition &condition, VisitedSet &visited) {
	typename Volume::Sampler sampler(volume);
	if (!sampler.setPosition(position)) {
		return 0;
	}
	int n = 0;
	// visit all connected voxels of the same color
	for (const glm::ivec3 &offset : voxel::arrayPathfinderFaces) {
		const glm::ivec3 &volPos = position + offset;
		if (!sampler.setPosition(volPos)) {
			continue;
		}
		if (!condition(sampler)) {
			continue;
		}
		if (!visited.insert(volPos)) {
			continue;
		}
		visitor(volPos.x, volPos.y, volPos.z, voxel);
		++n;
		n += visitConnectedByVoxel_r(volume, voxel, volPos, visitor, condition, visited);
	}
	return n;
}

template<class Volume, class Visitor = EmptyVisitor>
int visitConnectedByVoxel(const Volume &volume, const glm::ivec3 &position, Visitor &&visitor = Visitor()) {
	const voxel::Voxel voxel = volume.voxel(position);
	VisitedSet visited;
	VisitVoxelColor condition(voxel);
	return visitConnectedByVoxel_r(volume, voxel, position, visitor, condition, visited);
}

template<class Volume, class Visitor = EmptyVisitor, class Condition = VisitSolid>
int visitConnectedByCondition(const Volume &volume, const glm::ivec3 &position, Visitor &&visitor = Visitor(), Condition &&condition = Condition()) {
	const voxel::Voxel voxel = volume.voxel(position);
	VisitedSet visited;
	return visitConnectedByVoxel_r(volume, voxel, position, visitor, condition, visited);
}

} // namespace voxelutil
