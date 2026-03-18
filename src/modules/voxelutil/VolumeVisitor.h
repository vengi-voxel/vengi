/**
 * @file
 */

#pragma once

#include "app/ForParallel.h"
#include "color/ColorUtil.h"
#include "core/GLM.h"
#include "core/Trace.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/DynamicSet.h"
#include "core/concurrent/Atomic.h"
#include "palette/Palette.h"
#include "voxel/Connectivity.h"
#include "math/Axis.h"
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

struct VisitSolidFlag {
	const uint8_t _flag;
	VisitSolidFlag(uint8_t flag) : _flag(flag) {
	}

	template<class Sampler>
	inline bool operator()(const Sampler &sampler) const {
		if (isAir(sampler.voxel().getMaterial())) {
			return false;
		}
		return (sampler.voxel().getFlags() & _flag) == 0u;
	}
};

struct VisitSolidOutline : public VisitSolidFlag {
	VisitSolidOutline() : VisitSolidFlag(voxel::FlagOutline) {
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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
					const voxel::Voxel voxel = sampler3.voxel();
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

/**
 * @brief Condition for flat-surface flood fill.
 *
 * A voxel qualifies if it is solid, its given face is exposed (the neighbor in the face
 * direction is air), and its coordinate along the face axis is within @p deviation of the
 * start voxel. Pass as the Condition argument to visitFlatSurface.
 */
struct VisitFlatSurface {
	voxel::FaceBits _faceBit;
	int _faceAxis;    ///< 0=X, 1=Y, 2=Z
	int _startCoord;  ///< start position coordinate on the face axis
	int _deviation;   ///< max allowed distance from start along the face axis

	VisitFlatSurface(voxel::FaceNames face, const glm::ivec3 &startPos, int deviation = 0)
		: _faceBit(voxel::faceBits(face)),
		  _faceAxis(math::getIndexForAxis(voxel::faceToAxis(face))),
		  _startCoord(startPos[_faceAxis]),
		  _deviation(deviation) {}

	template<class Sampler>
	bool operator()(const Sampler &sampler) const {
		if (voxel::isAir(sampler.voxel().getMaterial())) {
			return false;
		}
		if (glm::abs(sampler.position()[_faceAxis] - _startCoord) > _deviation) {
			return false;
		}
		return (voxel::visibleFaces(sampler) & _faceBit) != voxel::FaceBits::None;
	}
};

/**
 * @brief Flood-fill starting at @p position, visiting connected voxels accepted by @p condition.
 *
 * Explores the 4 neighbors perpendicular to @p face. When @p deviation > 0 the search also
 * probes candidates at offset face-axis coordinates (within [startCoord+/-deviation]) so small
 * height steps can be traversed. Use visitFlatSurface(volume, position, face, deviation, visitor)
 * for the common case; this overload allows a custom condition functor.
 *
 * @param volume     The volume to query
 * @param position   Start voxel position (must satisfy condition)
 * @param face       Defines the perpendicular plane and deviation search direction
 * @param deviation  How many units above/below start to also consider at each step
 * @param visitor    Called with (x, y, z, voxel) for every accepted voxel including start
 * @param condition  Callable(Sampler) -> bool
 * @return Number of voxels visited
 */
template<class Volume, class Visitor, class Condition>
int visitFlatSurface(const Volume &volume, const glm::ivec3 &position, voxel::FaceNames face,
					 int deviation, Visitor &&visitor, Condition &&condition) {
	if (face == voxel::FaceNames::Max) {
		return 0;
	}
	typename Volume::Sampler startSampler(volume);
	if (!startSampler.setPosition(position)) {
		return 0;
	}
	if (!condition(startSampler)) {
		return 0;
	}
	const int faceAxis   = math::getIndexForAxis(voxel::faceToAxis(face));
	const int startCoord = position[faceAxis];
	// 4 directions perpendicular to the face normal
	const int perpAxis1 = (faceAxis + 1) % 3;
	const int perpAxis2 = (faceAxis + 2) % 3;
	glm::ivec3 perpOffsets[4] = {};
	perpOffsets[0][perpAxis1] =  1;
	perpOffsets[1][perpAxis1] = -1;
	perpOffsets[2][perpAxis2] =  1;
	perpOffsets[3][perpAxis2] = -1;
	VisitedSet visited;
	visited.insert(position);
	visitor(position.x, position.y, position.z, startSampler.voxel());
	int n = 1;
	constexpr size_t InitialQueueReserve = 64u;
	core::DynamicArray<glm::ivec3> queue;
	queue.reserve(InitialQueueReserve);
	queue.push_back(position);
	typename Volume::Sampler sampler(volume);
	while (!queue.empty()) {
		const glm::ivec3 current = queue.back();
		queue.pop();
		for (int i = 0; i < 4; ++i) {
			const glm::ivec3 neighborBase = current + perpOffsets[i];
			// Probe same level first, then +/-1, +/-2 ... up to deviation from start.
			for (int step = 0; step <= deviation; ++step) {
				bool claimedAtStep = false;
				const int ds[2] = {step, -step};
				const int count  = (step == 0) ? 1 : 2;
				for (int j = 0; j < count; ++j) {
					glm::ivec3 neighbor = neighborBase;
					neighbor[faceAxis]  = startCoord + ds[j];
					if (!visited.insert(neighbor)) {
						claimedAtStep = true;
						break;
					}
					if (!sampler.setPosition(neighbor)) {
						continue;
					}
					if (!condition(sampler)) {
						continue;
					}
					visitor(neighbor.x, neighbor.y, neighbor.z, sampler.voxel());
					++n;
					queue.push_back(neighbor);
					claimedAtStep = true;
					break;
				}
				if (claimedAtStep) {
					break;
				}
			}
		}
	}
	return n;
}

/// Convenience overload: constructs a VisitFlatSurface condition with the given deviation.
template<class Volume, class Visitor = EmptyVisitor>
int visitFlatSurface(const Volume &volume, const glm::ivec3 &position, voxel::FaceNames face,
					 int deviation = 0, Visitor &&visitor = Visitor()) {
	return visitFlatSurface(volume, position, face, deviation,
							core::forward<Visitor>(visitor),
							VisitFlatSurface(face, position, deviation));
}

/**
 * @brief Flood-fill along a surface using a global slope plane to accept/reject voxels.
 *
 * The clicked face defines a "height" axis. A slope plane is fitted once from the seed's
 * neighborhood using 2D linear regression, then frozen. The BFS expands outward and checks
 * each candidate voxel against the frozen plane: if its actual height deviates from the
 * plane's predicted height by more than @p maxDeviation, it is rejected.
 *
 * This approach is robust at surface edges (no per-voxel gradient needed) and naturally
 * handles L-shaped staircase kinks (they are small deviations on a well-fitted plane).
 * Freezing the plane prevents gradual drift around sharp angle changes.
 *
 * Uses 26-connectivity so the BFS can traverse diagonal staircase steps.
 *
 * @param volume          The volume to query
 * @param position        Start voxel position (must be solid surface voxel on @p face)
 * @param face            The clicked face - defines which axis is "height"
 * @param maxDeviation    Maximum allowed height deviation (in voxels) from the global slope plane
 * @param sampleDistance  How many voxels to probe around the seed to compute the initial plane
 * @param visitor         Called with (x, y, z, voxel) for every accepted voxel including start
 * @return Number of voxels visited
 */
template<class Volume, class Visitor = EmptyVisitor>
int visitSlopeSurface(const Volume &volume, const glm::ivec3 &position, voxel::FaceNames face,
					  int maxDeviation, int sampleDistance, Visitor &&visitor = Visitor()) {
	if (face == voxel::FaceNames::Max) {
		return 0;
	}
	typename Volume::Sampler sampler(volume);
	if (!sampler.setPosition(position)) {
		return 0;
	}
	if (voxel::isAir(sampler.voxel().getMaterial())) {
		return 0;
	}
	const voxel::FaceBits startFaces = voxel::visibleFaces(sampler);
	if (startFaces == voxel::FaceBits::None) {
		return 0;
	}

	const int heightAxis = math::getIndexForAxis(voxel::faceToAxis(face));
	const int uAxis = (heightAxis + 1) % 3;
	const int vAxis = (heightAxis + 2) % 3;
	const bool positiveNormal = voxel::isPositiveFace(face);
	const voxel::Region &volRegion = volume.region();

	// Find the outermost surface height at a given (u, v) coordinate.
	// Scans from outside inward to avoid hitting interior shell surfaces.
	const int searchRange = sampleDistance * 2;
	auto findSurfaceHeight = [&](int uCoord, int vCoord, int expectedHeight, int &outHeight) -> bool {
		const int startH = expectedHeight + (positiveNormal ? searchRange : -searchRange);
		const int endH = expectedHeight + (positiveNormal ? -searchRange : searchRange);
		const int stepH = positiveNormal ? -1 : 1;
		for (int scanH = startH; (positiveNormal ? (scanH >= endH) : (scanH <= endH)); scanH += stepH) {
			glm::ivec3 probe;
			probe[heightAxis] = scanH;
			probe[uAxis] = uCoord;
			probe[vAxis] = vCoord;
			if (!volRegion.containsPoint(probe)) {
				continue;
			}
			if (voxel::isAir(volume.voxel(probe).getMaterial())) {
				continue;
			}
			glm::ivec3 above = probe;
			above[heightAxis] += positiveNormal ? 1 : -1;
			if (!volRegion.containsPoint(above) || voxel::isAir(volume.voxel(above).getMaterial())) {
				outHeight = scanH;
				return true;
			}
		}
		return false;
	};

	// Running 2D linear regression: fits h = a + b*u + c*v to all accepted surface voxels.
	// Uses relative coordinates (du = u - seedU, dv = v - seedV) for numerical stability.
	// The plane predicts height at any (u,v): predictedH = seedH + gradU*du + gradV*dv
	const int seedU = position[uAxis];
	const int seedV = position[vAxis];
	const int seedH = position[heightAxis];

	// Accumulators for 2D regression: h = gradU*du + gradV*dv + intercept
	// Normal equations: [Σdu² Σdu·dv] [gradU]   [Σdu·dh]
	//                   [Σdu·dv Σdv²] [gradV] = [Σdv·dh]
	double sumDU = 0.0, sumDV = 0.0, sumDH = 0.0;
	double sumDU2 = 0.0, sumDV2 = 0.0, sumDUDV = 0.0;
	double sumDUDH = 0.0, sumDVDH = 0.0;
	int planeN = 0;
	float gradU = 0.0f;
	float gradV = 0.0f;

	auto addToPlane = [&](int uCoord, int vCoord, int height) {
		const double du = static_cast<double>(uCoord - seedU);
		const double dv = static_cast<double>(vCoord - seedV);
		const double dh = static_cast<double>(height - seedH);
		sumDU += du;
		sumDV += dv;
		sumDH += dh;
		sumDU2 += du * du;
		sumDV2 += dv * dv;
		sumDUDV += du * dv;
		sumDUDH += du * dh;
		sumDVDH += dv * dh;
		++planeN;
	};

	static constexpr int MinPlaneSamples = 3;
	static constexpr double MinDeterminant = 0.001;

	auto updatePlaneGradients = [&]() {
		if (planeN < MinPlaneSamples) {
			return;
		}
		// Solve normal equations for 2D regression (with intercept absorbed by centering)
		const double sampleCount = static_cast<double>(planeN);
		const double meanDU = sumDU / sampleCount;
		const double meanDV = sumDV / sampleCount;
		const double meanDH = sumDH / sampleCount;
		// Centered second moments
		const double suu = sumDU2 - sampleCount * meanDU * meanDU;
		const double svv = sumDV2 - sampleCount * meanDV * meanDV;
		const double suv = sumDUDV - sampleCount * meanDU * meanDV;
		const double suh = sumDUDH - sampleCount * meanDU * meanDH;
		const double svh = sumDVDH - sampleCount * meanDV * meanDH;
		const double det = suu * svv - suv * suv;
		if (glm::abs(det) > MinDeterminant) {
			gradU = static_cast<float>((svv * suh - suv * svh) / det);
			gradV = static_cast<float>((suu * svh - suv * suh) / det);
		}
	};

	auto predictHeight = [&](int uCoord, int vCoord) -> float {
		return static_cast<float>(seedH) + gradU * static_cast<float>(uCoord - seedU) + gradV * static_cast<float>(vCoord - seedV);
	};

	// Bootstrap the plane from the seed's neighborhood
	for (int du = -sampleDistance; du <= sampleDistance; ++du) {
		for (int dv = -sampleDistance; dv <= sampleDistance; ++dv) {
			int surfaceHeight = 0;
			if (du == 0 && dv == 0) {
				surfaceHeight = seedH;
			} else if (!findSurfaceHeight(seedU + du, seedV + dv, seedH, surfaceHeight)) {
				continue;
			}
			addToPlane(seedU + du, seedV + dv, surfaceHeight);
		}
	}
	updatePlaneGradients();

	const float maxDev = static_cast<float>(maxDeviation);

	VisitedSet visited;
	visited.insert(position);
	visitor(position.x, position.y, position.z, sampler.voxel());
	int nVisited = 1;

	constexpr size_t InitialQueueReserve = 64u;
	core::DynamicArray<glm::ivec3> queue;
	queue.reserve(InitialQueueReserve);
	queue.push_back(position);

	// 26-connectivity: staircase steps are edge/corner-connected, not face-connected.
	// Uses face/edge/corner offset arrays from voxel::Connectivity.h
	while (!queue.empty()) {
		const glm::ivec3 current = queue.back();
		queue.pop();
		auto visitNeighbor = [&](const glm::ivec3 &offset) {
			const glm::ivec3 neighborPos = current + offset;
			if (!visited.insert(neighborPos)) {
				return;
			}
			if (!volRegion.containsPoint(neighborPos)) {
				return;
			}
			if (voxel::isAir(volume.voxel(neighborPos).getMaterial())) {
				return;
			}
			// Must be a surface voxel (has the clicked face exposed)
			glm::ivec3 above = neighborPos;
			above[heightAxis] += positiveNormal ? 1 : -1;
			if (volRegion.containsPoint(above) && !voxel::isAir(volume.voxel(above).getMaterial())) {
				return;
			}
			// Check if this voxel's height fits the slope plane (frozen after bootstrap)
			const float predicted = predictHeight(neighborPos[uAxis], neighborPos[vAxis]);
			const float actual = static_cast<float>(neighborPos[heightAxis]);
			if (glm::abs(actual - predicted) > maxDev) {
				return;
			}
			if (!sampler.setPosition(neighborPos)) {
				return;
			}
			visitor(neighborPos.x, neighborPos.y, neighborPos.z, sampler.voxel());
			++nVisited;
			queue.push_back(neighborPos);
		};
		for (const glm::ivec3 &offset : voxel::arrayPathfinderFaces) {
			visitNeighbor(offset);
		}
		for (const glm::ivec3 &offset : voxel::arrayPathfinderEdges) {
			visitNeighbor(offset);
		}
		for (const glm::ivec3 &offset : voxel::arrayPathfinderCorners) {
			visitNeighbor(offset);
		}
	}
	return nVisited;
}

} // namespace voxelutil
