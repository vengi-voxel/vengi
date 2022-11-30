/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/Trace.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"

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
	mYZX,
	YZmX,
	Max
};

/**
 * @brief Will skip air voxels on volume
 */
struct SkipEmpty {
	inline bool operator()(const voxel::Voxel &voxel) const {
		return !isAir(voxel.getMaterial());
	}
};

struct VisitAll {
	inline bool operator()(const voxel::Voxel &voxel) const {
		return true;
	}
};

template <class Volume, class Visitor, typename Condition = SkipEmpty>
int visitVolume(const Volume &volume, const voxel::Region &region, int xOff, int yOff, int zOff, Visitor &&visitor,
				Condition condition = Condition(), VisitorOrder order = VisitorOrder::ZYX) {
	core_trace_scoped(VisitVolume);
	int cnt = 0;

	typename Volume::Sampler sampler(volume);

	switch (order) {
	case VisitorOrder::XYZ:
		for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
			for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
				sampler.setPosition(x, y, region.getLowerZ());
				for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
					const voxel::Voxel &voxel = sampler.voxel();
					sampler.movePositiveZ(zOff);
					if (!condition(voxel)) {
						continue;
					}
					visitor(x, y, z, voxel);
					++cnt;
				}
			}
		}
		break;
	case VisitorOrder::ZYX:
		for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
			for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
				sampler.setPosition(region.getLowerX(), y, z);
				for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
					const voxel::Voxel &voxel = sampler.voxel();
					sampler.movePositiveX(xOff);
					if (!condition(voxel)) {
						continue;
					}
					visitor(x, y, z, voxel);
					++cnt;
				}
			}
		}
		break;
	case VisitorOrder::ZXY:
		for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
				sampler.setPosition(x, region.getLowerY(), z);
				for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
					const voxel::Voxel &voxel = sampler.voxel();
					sampler.movePositiveY(yOff);
					if (!condition(voxel)) {
						continue;
					}
					visitor(x, y, z, voxel);
					++cnt;
				}
			}
		}
		break;
	case VisitorOrder::XZY:
		for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
			for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
				sampler.setPosition(x, region.getLowerY(), z);
				for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
					const voxel::Voxel &voxel = sampler.voxel();
					sampler.movePositiveY(yOff);
					if (!condition(voxel)) {
						continue;
					}
					visitor(x, y, z, voxel);
					++cnt;
				}
			}
		}
		break;
	case VisitorOrder::XZmY:
		for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
			for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
				sampler.setPosition(x, region.getUpperY(), z);
				for (int32_t y = region.getUpperY(); y >= region.getLowerY(); y -= yOff) {
					const voxel::Voxel &voxel = sampler.voxel();
					sampler.moveNegativeY(yOff);
					if (!condition(voxel)) {
						continue;
					}
					visitor(x, y, z, voxel);
					++cnt;
				}
			}
		}
		break;
	case VisitorOrder::mXmZY:
		for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
			for (int32_t z = region.getUpperZ(); z >= region.getLowerZ(); z -= zOff) {
				sampler.setPosition(x, region.getLowerY(), z);
				for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
					const voxel::Voxel &voxel = sampler.voxel();
					sampler.movePositiveY(yOff);
					if (!condition(voxel)) {
						continue;
					}
					visitor(x, y, z, voxel);
					++cnt;
				}
			}
		}
		break;
	case VisitorOrder::mXZY:
		for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
			for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
				sampler.setPosition(x, region.getLowerY(), z);
				for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
					const voxel::Voxel &voxel = sampler.voxel();
					sampler.movePositiveY(yOff);
					if (!condition(voxel)) {
						continue;
					}
					visitor(x, y, z, voxel);
					++cnt;
				}
			}
		}
		break;
	case VisitorOrder::XmZY:
		for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
			for (int32_t z = region.getUpperZ(); z >= region.getLowerZ(); z -= zOff) {
				sampler.setPosition(x, region.getLowerY(), z);
				for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
					const voxel::Voxel &voxel = sampler.voxel();
					sampler.movePositiveY(yOff);
					if (!condition(voxel)) {
						continue;
					}
					visitor(x, y, z, voxel);
					++cnt;
				}
			}
		}
		break;
	case VisitorOrder::XmZmY:
		for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
			for (int32_t z = region.getUpperZ(); z >= region.getLowerZ(); z -= zOff) {
				sampler.setPosition(x, region.getUpperY(), z);
				for (int32_t y = region.getUpperY(); y >= region.getLowerY(); y -= yOff) {
					const voxel::Voxel &voxel = sampler.voxel();
					sampler.moveNegativeY(yOff);
					if (!condition(voxel)) {
						continue;
					}
					visitor(x, y, z, voxel);
					++cnt;
				}
			}
		}
		break;
	case VisitorOrder::mXmZmY:
		for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
			for (int32_t z = region.getUpperZ(); z >= region.getLowerZ(); z -= zOff) {
				sampler.setPosition(x, region.getUpperY(), z);
				for (int32_t y = region.getUpperY(); y >= region.getLowerY(); y -= yOff) {
					const voxel::Voxel &voxel = sampler.voxel();
					sampler.moveNegativeY(yOff);
					if (!condition(voxel)) {
						continue;
					}
					visitor(x, y, z, voxel);
					++cnt;
				}
			}
		}
		break;
	case VisitorOrder::mXZmY:
		for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
			for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
				sampler.setPosition(x, region.getUpperY(), z);
				for (int32_t y = region.getUpperY(); y >= region.getLowerY(); y -= yOff) {
					const voxel::Voxel &voxel = sampler.voxel();
					sampler.moveNegativeY(yOff);
					if (!condition(voxel)) {
						continue;
					}
					visitor(x, y, z, voxel);
					++cnt;
				}
			}
		}
		break;
	case VisitorOrder::YXZ:
		for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
				sampler.setPosition(x, y, region.getLowerZ());
				for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
					const voxel::Voxel &voxel = sampler.voxel();
					sampler.movePositiveZ(zOff);
					if (!condition(voxel)) {
						continue;
					}
					visitor(x, y, z, voxel);
					++cnt;
				}
			}
		}
		break;
	case VisitorOrder::YZX:
		for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
			for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
				sampler.setPosition(region.getLowerX(), y, z);
				for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += xOff) {
					const voxel::Voxel &voxel = sampler.voxel();
					sampler.movePositiveX(xOff);
					if (!condition(voxel)) {
						continue;
					}
					visitor(x, y, z, voxel);
					++cnt;
				}
			}
		}
		break;
	case VisitorOrder::YZmX:
		for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += yOff) {
			for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
				sampler.setPosition(region.getUpperX(), y, z);
				for (int32_t x = region.getUpperX(); x >= region.getLowerX(); x -= xOff) {
					const voxel::Voxel &voxel = sampler.voxel();
					sampler.moveNegativeX(xOff);
					if (!condition(voxel)) {
						continue;
					}
					visitor(x, y, z, voxel);
					++cnt;
				}
			}
		}
		break;
	case VisitorOrder::mYZX:
		for (int32_t y = region.getUpperY(); y <= region.getLowerY(); y += yOff) {
			for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += zOff) {
				sampler.setPosition(region.getLowerX(), y, z);
				for (int32_t x = region.getLowerX(); x >= region.getUpperX(); x -= xOff) {
					const voxel::Voxel &voxel = sampler.voxel();
					sampler.moveNegativeX(xOff);
					if (!condition(voxel)) {
						continue;
					}
					visitor(x, y, z, voxel);
					++cnt;
				}
			}
		}
		break;
	case VisitorOrder::Max:
		break;
	}

#undef LOOP
	return cnt;
}

template <class Volume, class Visitor, typename Condition = SkipEmpty>
int visitVolume(const Volume &volume, int xOff, int yOff, int zOff, Visitor &&visitor, Condition condition = Condition(), VisitorOrder order = VisitorOrder::ZYX) {
	const voxel::Region &region = volume.region();
	return visitVolume(volume, region, xOff, yOff, zOff, visitor, condition, order);
}

template <class Volume, class Visitor, typename Condition = SkipEmpty>
int visitVolume(const Volume &volume, Visitor &&visitor, Condition condition = Condition(), VisitorOrder order = VisitorOrder::ZYX) {
	return visitVolume(volume, 1, 1, 1, visitor, condition, order);
}

template <class Volume, class Visitor, typename Condition = SkipEmpty>
int visitVolume(const Volume &volume, const voxel::Region &region, Visitor &&visitor, Condition condition = Condition(), VisitorOrder order = VisitorOrder::ZYX) {
	return visitVolume(volume, region, 1, 1, 1, visitor, condition, order);
}

template <class Volume, class Visitor>
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

} // namespace voxelutil
