/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/GLM.h"
#include "core/Trace.h"
#include "core/collection/DynamicSet.h"
#include "math/Axis.h"
#include "voxel/Connectivity.h"
#include "voxel/Face.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

namespace voxelutil {

enum class VisitorOrder {
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
struct SkipEmpty {
	inline bool operator()(const voxel::Voxel &voxel) const {
		return !isAir(voxel.getMaterial());
	}
};

/**
 * @note Visitor condition
 */
struct VisitAll {
	inline bool operator()(const voxel::Voxel &voxel) const {
		return true;
	}
};

/**
 * @note Visitor condition
 */
struct VisitEmpty {
	inline bool operator()(const voxel::Voxel &voxel) const {
		return isAir(voxel.getMaterial());
	}
};

/**
 * @note Visitor condition
 */
struct VisitColor {
	const uint8_t _color;
	VisitColor(uint8_t color) : _color(color) {
	}
	inline bool operator()(const voxel::Voxel &voxel) const {
		if (voxel.getMaterial() == voxel::VoxelType::Air) {
			return false;
		}
		return voxel.getColor() == _color;
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
	if (!condition(voxel)) {                                                                                           \
		continue;                                                                                                      \
	}                                                                                                                  \
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

template<class Volume, class Visitor, typename Condition = SkipEmpty>
int visitVolume(const Volume &volume, const voxel::Region &region, int xOff, int yOff, int zOff, Visitor &&visitor,
				Condition condition = Condition(), VisitorOrder order = VisitorOrder::ZYX) {
	core_trace_scoped(VisitVolume);
	int cnt = 0;

	typename Volume::Sampler sampler(volume);

	switch (order) {
	case VisitorOrder::XYZ:
		sampler.setPosition(region.getLowerCorner());
		for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t y = region.getUpperY(); y >= region.getLowerY(); y -= yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t z = region.getUpperZ(); z >= region.getLowerZ(); z -= zOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t z = region.getUpperZ(); z >= region.getLowerZ(); z -= zOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t z = region.getUpperZ(); z >= region.getLowerZ(); z -= zOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t y = region.getUpperY(); y >= region.getLowerY(); y -= yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t z = region.getUpperZ(); z >= region.getLowerZ(); z -= zOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t y = region.getUpperY(); y >= region.getLowerY(); y -= yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t y = region.getUpperY(); y >= region.getLowerY(); y -= yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t y = region.getUpperY(); y >= region.getLowerY(); y -= yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t z = region.getUpperZ(); z >= region.getLowerZ(); z -= zOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t z = region.getUpperZ(); z >= region.getLowerZ(); z -= zOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t z = region.getUpperZ(); z >= region.getLowerZ(); z -= zOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t y = region.getUpperY(); y >= region.getLowerY(); y -= yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t y = region.getUpperY(); y >= region.getLowerY(); y -= yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t z = region.getUpperZ(); z >= region.getLowerZ(); z -= zOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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
			typename Volume::Sampler sampler2 = sampler;
			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
				typename Volume::Sampler sampler3 = sampler2;
				for (int32_t y = region.getUpperY(); y >= region.getLowerY(); y -= yOff) {
					const voxel::Voxel &voxel = sampler3.voxel();
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

template<class Volume, class Visitor, typename Condition = SkipEmpty>
int visitVolume(const Volume &volume, int xOff, int yOff, int zOff, Visitor &&visitor,
				Condition condition = Condition(), VisitorOrder order = VisitorOrder::ZYX) {
	const voxel::Region &region = volume.region();
	return visitVolume(volume, region, xOff, yOff, zOff, visitor, condition, order);
}

template<class Volume, class Visitor, typename Condition = SkipEmpty>
int visitVolume(const Volume &volume, Visitor &&visitor, Condition condition = Condition(),
				VisitorOrder order = VisitorOrder::ZYX) {
	return visitVolume(volume, 1, 1, 1, visitor, condition, order);
}

template<class Volume, class Visitor, typename Condition = SkipEmpty>
int visitVolume(const Volume &volume, const voxel::Region &region, Visitor &&visitor, Condition condition = Condition(),
				VisitorOrder order = VisitorOrder::ZYX) {
	return visitVolume(volume, region, 1, 1, 1, visitor, condition, order);
}

template<class Volume, class Visitor>
int visitSurfaceVolume(const Volume &volume, Visitor &&visitor, VisitorOrder order = VisitorOrder::ZYX) {
	int cnt = 0;
	const auto hullVisitor = [&cnt, &volume, visitor](int x, int y, int z, const voxel::Voxel &voxel) {
		if (visibleFaces(volume, x, y, z) != voxel::FaceBits::None) {
			visitor(x, y, z, voxel);
			++cnt;
		}
	};
	visitVolume(volume, hullVisitor, SkipEmpty(), order);
	return cnt;
}

/**
 * @return The number of voxels visited.
 */
template<class Volume, class Visitor>
int visitFace(const Volume &volume, const voxel::Region &region, voxel::FaceNames face, Visitor &&visitor, VisitorOrder order,
			  bool searchSurface = false) {
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
template<class Volume, class Visitor>
int visitFace(const Volume &volume, const voxel::Region &region, voxel::FaceNames face, Visitor &&visitor,
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

template<class Volume, class Visitor>
int visitFace(const Volume &volume, voxel::FaceNames face, Visitor &&visitor, VisitorOrder order = VisitorOrder::Max, bool searchSurface = false) {
	const voxel::Region &region = volume.region();
	if (order == VisitorOrder::Max) {
		return visitFace(volume, region, face, visitor, searchSurface);
	}
	return visitFace(volume, region, face, visitor, order, searchSurface);
}

/**
 * @brief Visit all non-visible voxels - voxels that are surrounded by solid voxels on all sides
 * @sa visibleFaces()
 */
template<class Volume, class Visitor>
int visitUndergroundVolume(const Volume &volume, Visitor &&visitor, VisitorOrder order = VisitorOrder::ZYX) {
	int cnt = 0;
	const auto hullVisitor = [&cnt, &volume, visitor](int x, int y, int z, const voxel::Voxel &voxel) {
		if (visibleFaces(volume, x, y, z) == voxel::FaceBits::None) {
			visitor(x, y, z, voxel);
			++cnt;
		}
	};
	visitVolume(volume, hullVisitor, SkipEmpty(), order);
	return cnt;
}

typedef core::DynamicSet<glm::ivec3, 1031, glm::hash<glm::ivec3>> VisitedSet;

template<class Volume, class Visitor>
static int visitConnectedByVoxel_r(const Volume &volume, const voxel::Voxel &voxel, const glm::ivec3 &position, Visitor &visitor, VisitedSet &visited) {
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
		if (!sampler.voxel().isSame(voxel)) {
			continue;
		}
		if (!visited.insert(volPos)) {
			continue;
		}
		visitor(volPos.x, volPos.y, volPos.z, voxel);
		++n;
		n += visitConnectedByVoxel_r(volume, voxel, volPos, visitor, visited);
	}
	return n;
}

template<class Volume, class Visitor>
int visitConnectedByVoxel(const Volume &volume, const glm::ivec3 &position, Visitor &&visitor) {
	const voxel::Voxel voxel = volume.voxel(position);
	VisitedSet visited;
	return visitConnectedByVoxel_r(volume, voxel, position, visitor, visited);
}

} // namespace voxelutil
