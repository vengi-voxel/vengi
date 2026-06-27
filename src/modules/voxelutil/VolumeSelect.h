/**
 * @file
 */

#pragma once

#include "core/GLM.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/DynamicSet.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include <glm/common.hpp>
#include <glm/vec3.hpp>

namespace voxelutil {

/**
 * @brief Point-in-polygon test using ray casting in 2D face-plane coordinates
 * @param path Polygon vertices
 * @param pu U-axis coordinate of the test point
 * @param pv V-axis coordinate of the test point
 * @param uAxis Index of the U axis in ivec3 (0=x,1=y,2=z)
 * @param vAxis Index of the V axis in ivec3
 */
bool lassoContains(const core::DynamicArray<glm::ivec3> &path, int pu, int pv, int uAxis, int vAxis);

/**
 * @brief A solid (non-air) voxel inside @p region.
 */
template<typename Volume>
inline bool selectIsSolid(const Volume &volume, const glm::ivec3 &p, const voxel::Region &region) {
	if (!region.containsPoint(p)) {
		return false;
	}
	return !voxel::isAir(volume.voxel(p.x, p.y, p.z).getMaterial());
}

/**
 * @brief Front-most solid voxel depth in column (@p u, @p v), scanning inward from the clicked face.
 *
 * Early-outs at the first solid, so it never walks the whole column on a surface near the face.
 * @param outW Receives the depth of the visible (front-most) surface voxel.
 * @return true when the column contains a solid voxel.
 */
template<typename Volume>
bool columnFrontSurface(const Volume &volume, int u, int v, int uAxis, int vAxis, int wAxis, bool positiveNormal,
						const voxel::Region &region, int &outW) {
	glm::ivec3 pos(0);
	pos[uAxis] = u;
	pos[vAxis] = v;
	const int wLo = region.getLowerCorner()[wAxis];
	const int wHi = region.getUpperCorner()[wAxis];
	if (positiveNormal) {
		for (int w = wHi; w >= wLo; --w) {
			pos[wAxis] = w;
			if (selectIsSolid(volume, pos, region)) {
				outW = w;
				return true;
			}
		}
	} else {
		for (int w = wLo; w <= wHi; ++w) {
			pos[wAxis] = w;
			if (selectIsSolid(volume, pos, region)) {
				outW = w;
				return true;
			}
		}
	}
	return false;
}

// Forward declarations: these helper templates are defined later in this header.
template<typename PlotFunc>
void bresenham3d(const glm::ivec3 &a, const glm::ivec3 &b, PlotFunc &&plot);
template<typename Volume, typename MarkFunc>
void lineMarkSolid(const Volume &volume, const glm::ivec3 &a, const glm::ivec3 &b, int width,
				   const voxel::Region &region, MarkFunc &&markFunc);
template<typename Volume, typename MarkFunc>
void lineDrapeSurface(const Volume &volume, const glm::ivec3 &a, const glm::ivec3 &b, int uAxis, int vAxis, int wAxis,
					  bool positiveNormal, int width, const voxel::Region &region, MarkFunc &&markFunc);
template<typename Volume>
bool findSurfaceNear(const Volume &volume, int u, int v, int refW, int tolerance, int uAxis, int vAxis, int wAxis,
					 bool positiveNormal, const voxel::Region &region, int &outW);

/** Lateral neighbour offsets (4 face + 4 diagonal) used to bridge terrace risers in the lasso fill. */
static const int LassoRiserNeighbourCount = 8;
static const int LassoRiserNeighbourU[LassoRiserNeighbourCount] = {1, -1, 0, 0, 1, 1, -1, -1};
static const int LassoRiserNeighbourV[LassoRiserNeighbourCount] = {0, 0, 1, -1, 1, -1, 1, -1};

/**
 * @brief Select the visible surface inside a lasso polygon, filling its whole interior.
 */
template<typename Volume, typename MarkFunc>
void lassoFloodFillSurface(const Volume &volume, const core::DynamicArray<glm::ivec3> &path, int uAxis, int vAxis,
						   int wAxis, bool positiveNormal, const voxel::Region &region, MarkFunc &&markFunc,
						   int maxDeviation) {
	(void)maxDeviation;
	if (path.size() < 2) {
		return;
	}

	auto inPolygon = [&](int u, int v) { return lassoContains(path, u, v, uAxis, vAxis); };
	auto columnFront = [&](int u, int v, int &outW) -> bool {
		return columnFrontSurface(volume, u, v, uAxis, vAxis, wAxis, positiveNormal, region, outW);
	};
	auto markAt = [&](int u, int v, int w) {
		glm::ivec3 q(0);
		q[uAxis] = u;
		q[vAxis] = v;
		q[wAxis] = w;
		const voxel::Voxel vox = volume.voxel(q.x, q.y, q.z);
		markFunc(q.x, q.y, q.z, vox);
	};

	auto markColumn = [&](int u, int v) {
		int d;
		if (!columnFront(u, v, d)) {
			return;
		}
		markAt(u, v, d);
		for (int k = 0; k < LassoRiserNeighbourCount; ++k) {
			const int nu = u + LassoRiserNeighbourU[k];
			const int nv = v + LassoRiserNeighbourV[k];
			if (!inPolygon(nu, nv)) {
				continue;
			}
			int dn;
			if (!columnFront(nu, nv, dn)) {
				continue;
			}
			if (positiveNormal) {
				for (int w = d - 1; w > dn; --w) {
					glm::ivec3 q(0);
					q[uAxis] = u;
					q[vAxis] = v;
					q[wAxis] = w;
					if (selectIsSolid(volume, q, region)) {
						markAt(u, v, w);
					}
				}
			} else {
				for (int w = d + 1; w < dn; ++w) {
					glm::ivec3 q(0);
					q[uAxis] = u;
					q[vAxis] = v;
					q[wAxis] = w;
					if (selectIsSolid(volume, q, region)) {
						markAt(u, v, w);
					}
				}
			}
		}
	};

	const int vertexCount = (int)path.size();
	for (int i = 0; i < vertexCount; ++i) {
		glm::ivec3 a2 = path[i];
		glm::ivec3 b2 = path[(i + 1) % vertexCount];
		a2[wAxis] = 0;
		b2[wAxis] = 0;
		bresenham3d(a2, b2, [&](const glm::ivec3 &c) { markColumn(c[uAxis], c[vAxis]); });
	}

	if (path.size() < 3) {
		return;
	}

	int minU = path[0][uAxis];
	int maxU = path[0][uAxis];
	int minV = path[0][vAxis];
	int maxV = path[0][vAxis];
	for (const glm::ivec3 &p : path) {
		minU = glm::min(minU, p[uAxis]);
		maxU = glm::max(maxU, p[uAxis]);
		minV = glm::min(minV, p[vAxis]);
		maxV = glm::max(maxV, p[vAxis]);
	}
	for (int u = minU; u <= maxU; ++u) {
		for (int v = minV; v <= maxV; ++v) {
			if (inPolygon(u, v)) {
				markColumn(u, v);
			}
		}
	}
}

/**
 * @brief Visit every integer cell on the 3D Bresenham line from @p a to @p b (inclusive).
 */
template<typename PlotFunc>
void bresenham3d(const glm::ivec3 &a, const glm::ivec3 &b, PlotFunc &&plot) {
	glm::ivec3 p = a;
	const glm::ivec3 d = glm::abs(b - a);
	const glm::ivec3 s(b.x >= a.x ? 1 : -1, b.y >= a.y ? 1 : -1, b.z >= a.z ? 1 : -1);
	if (d.x >= d.y && d.x >= d.z) {
		int e1 = 2 * d.y - d.x;
		int e2 = 2 * d.z - d.x;
		while (true) {
			plot(p);
			if (p.x == b.x) {
				break;
			}
			if (e1 >= 0) {
				p.y += s.y;
				e1 -= 2 * d.x;
			}
			if (e2 >= 0) {
				p.z += s.z;
				e2 -= 2 * d.x;
			}
			p.x += s.x;
			e1 += 2 * d.y;
			e2 += 2 * d.z;
		}
	} else if (d.y >= d.x && d.y >= d.z) {
		int e1 = 2 * d.x - d.y;
		int e2 = 2 * d.z - d.y;
		while (true) {
			plot(p);
			if (p.y == b.y) {
				break;
			}
			if (e1 >= 0) {
				p.x += s.x;
				e1 -= 2 * d.y;
			}
			if (e2 >= 0) {
				p.z += s.z;
				e2 -= 2 * d.y;
			}
			p.y += s.y;
			e1 += 2 * d.x;
			e2 += 2 * d.z;
		}
	} else {
		int e1 = 2 * d.y - d.z;
		int e2 = 2 * d.x - d.z;
		while (true) {
			plot(p);
			if (p.z == b.z) {
				break;
			}
			if (e1 >= 0) {
				p.y += s.y;
				e1 -= 2 * d.z;
			}
			if (e2 >= 0) {
				p.x += s.x;
				e2 -= 2 * d.z;
			}
			p.z += s.z;
			e1 += 2 * d.y;
			e2 += 2 * d.x;
		}
	}
}

/**
 * @brief Rasterize the straight 3D line @p a -> @p b and mark every solid voxel within @p width of the line.
 */
template<typename Volume, typename MarkFunc>
void lineMarkSolid(const Volume &volume, const glm::ivec3 &a, const glm::ivec3 &b, int width,
				   const voxel::Region &region, MarkFunc &&markFunc) {
	const int r = glm::max(0, width / 2);
	core::DynamicSet<glm::ivec3, 1031, glm::hash<glm::ivec3>> marked;
	auto plot = [&](const glm::ivec3 &c) {
		for (int dz = -r; dz <= r; ++dz) {
			for (int dy = -r; dy <= r; ++dy) {
				for (int dx = -r; dx <= r; ++dx) {
					const glm::ivec3 q(c.x + dx, c.y + dy, c.z + dz);
					if (!region.containsPoint(q)) {
						continue;
					}
					if (marked.has(q)) {
						continue;
					}
					const voxel::Voxel v = volume.voxel(q.x, q.y, q.z);
					if (voxel::isAir(v.getMaterial())) {
						continue;
					}
					marked.insert(q);
					markFunc(q.x, q.y, q.z, v);
				}
			}
		}
	};
	bresenham3d(a, b, plot);
}

/**
 * @brief Draw a line that drapes over the visible surface from @p a to @p b.
 */
template<typename Volume, typename MarkFunc>
void lineDrapeSurface(const Volume &volume, const glm::ivec3 &a, const glm::ivec3 &b, int uAxis, int vAxis, int wAxis,
					  bool positiveNormal, int width, const voxel::Region &region, MarkFunc &&markFunc) {
	const int r = glm::max(0, width / 2);
	auto axisPos = [&](int u, int v, int w) {
		glm::ivec3 c(0);
		c[uAxis] = u;
		c[vAxis] = v;
		c[wAxis] = w;
		return c;
	};
	auto markBall = [&](const glm::ivec3 &center) {
		for (int dz = -r; dz <= r; ++dz) {
			for (int dy = -r; dy <= r; ++dy) {
				for (int dx = -r; dx <= r; ++dx) {
					const glm::ivec3 q(center.x + dx, center.y + dy, center.z + dz);
					if (!selectIsSolid(volume, q, region)) {
						continue;
					}
					const voxel::Voxel vox = volume.voxel(q.x, q.y, q.z);
					markFunc(q.x, q.y, q.z, vox);
				}
			}
		}
	};

	glm::ivec3 a2 = a;
	glm::ivec3 b2 = b;
	a2[wAxis] = 0;
	b2[wAxis] = 0;
	bool havePrev = false;
	int prevU = 0;
	int prevV = 0;
	int prevW = 0;
	bresenham3d(a2, b2, [&](const glm::ivec3 &c) {
		const int u = c[uAxis];
		const int v = c[vAxis];
		int d;
		if (!columnFrontSurface(volume, u, v, uAxis, vAxis, wAxis, positiveNormal, region, d)) {
			havePrev = false;
			return;
		}
		markBall(axisPos(u, v, d));
		if (havePrev && (u != prevU || v != prevV)) {
			const int lo = glm::min(d, prevW);
			const int hi = glm::max(d, prevW);
			const bool currentTaller = positiveNormal ? (d >= prevW) : (d <= prevW);
			const int wallU = currentTaller ? u : prevU;
			const int wallV = currentTaller ? v : prevV;
			for (int w = lo; w <= hi; ++w) {
				markBall(axisPos(wallU, wallV, w));
			}
		}
		prevU = u;
		prevV = v;
		prevW = d;
		havePrev = true;
	});
}

/**
 * @brief In column (@p u, @p v), find the exposed surface voxel nearest to @p refW within @p tolerance.
 */
template<typename Volume>
bool findSurfaceNear(const Volume &volume, int u, int v, int refW, int tolerance, int uAxis, int vAxis, int wAxis,
					 bool positiveNormal, const voxel::Region &region, int &outW) {
	glm::ivec3 pos(0);
	pos[uAxis] = u;
	pos[vAxis] = v;
	auto isExposedSurface = [&](int w) {
		pos[wAxis] = w;
		if (!region.containsPoint(pos)) {
			return false;
		}
		if (voxel::isAir(volume.voxel(pos.x, pos.y, pos.z).getMaterial())) {
			return false;
		}
		glm::ivec3 np = pos;
		np[wAxis] = positiveNormal ? w + 1 : w - 1;
		if (!region.containsPoint(np)) {
			return true;
		}
		return voxel::isAir(volume.voxel(np.x, np.y, np.z).getMaterial());
	};
	for (int d = 0; d <= tolerance; ++d) {
		const int first = positiveNormal ? refW + d : refW - d;
		if (isExposedSurface(first)) {
			outW = first;
			return true;
		}
		if (d != 0) {
			const int second = positiveNormal ? refW - d : refW + d;
			if (isExposedSurface(second)) {
				outW = second;
				return true;
			}
		}
	}
	return false;
}

} // namespace voxelutil
