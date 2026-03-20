/**
 * @file
 */

#include "TextureSurfaceExtractor.h"
#include "core/GLMConst.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/collection/Buffer.h"
#include "math/Rect.h"
#include "palette/Palette.h"
#include "voxel/ChunkMesh.h"
#include "voxel/Face.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/VoxelVertex.h"
#include "voxel/external/stb_rect_pack.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace voxel {

namespace {

using IRect = math::Rect<int>;

struct TexturSurfaceMesherState {
	static constexpr int PADDING = 1;
	static constexpr int TEX_SIZE = 2048;

	core::Buffer<color::RGBA> colors;
	IRect rect{0, 0, 0, 0};
	stbrp_context context;
	stbrp_node nodes[TEX_SIZE];

	TexturSurfaceMesherState() {
		colors.resize(TEX_SIZE * TEX_SIZE);
		stbrp_init_target(&context, TEX_SIZE, TEX_SIZE, nodes, TEX_SIZE);
	}

	IRect add(const core::Buffer<color::RGBA> &grid, int gridH, const IRect &original) {
		const int w = original.width();
		const int h = original.height();

		stbrp_rect r;
		r.id = 0;
		r.w = w + PADDING;
		r.h = h + PADDING;
		r.was_packed = 0;

		stbrp_pack_rects(&context, &r, 1);

		if (r.was_packed == 0) {
			Log::warn("Texture atlas full!");
			return IRect(0, 0, 0, 0);
		}

		const int px = r.x;
		const int py = r.y;

		for (int gy = 0; gy < h; ++gy) {
			for (int gx = 0; gx < w; ++gx) {
				const int colorIdx = (py + gy) * TEX_SIZE + (px + gx);
				const int gridIdx = (original.getMinX() + gx) * gridH + (original.getMinZ() + gy);
				colors[colorIdx] = grid[gridIdx];
			}
		}

		if (px + w > rect.getMaxX()) {
			rect.setMaxX(px + w);
		}
		if (py + h > rect.getMaxZ()) {
			rect.setMaxZ(py + h);
		}

		return IRect(px, py, px + w, py + h);
	}
};

// O(w*h) largest-rectangle algorithm using histogram + stack approach.
// Scans column-by-column for cache-friendly access with mask[x * h + y] layout.
static void findLargestRect(const core::Buffer<color::RGBA> &mask, int w, int h,
							core::Buffer<int> &heights, core::Buffer<int> &stack, IRect &largest) {
	largest = IRect(0, 0, 0, 0);
	int bestArea = 0;

	for (int i = 0; i < h; ++i) {
		heights[i] = 0;
	}

	for (int x = 0; x < w; ++x) {
		for (int y = 0; y < h; ++y) {
			if (mask[x * h + y].a != 0) {
				heights[y]++;
			} else {
				heights[y] = 0;
			}
		}

		int top = -1;
		for (int y = 0; y <= h; ++y) {
			int curH = (y < h) ? heights[y] : 0;
			while (top >= 0 && heights[stack[top]] > curH) {
				int idx = stack[top--];
				int rectW = heights[idx];
				int rectH = (top >= 0) ? (y - stack[top] - 1) : y;
				int area = rectW * rectH;
				if (area > bestArea) {
					bestArea = area;
					int startX = x - rectW + 1;
					int startY = (top >= 0) ? (stack[top] + 1) : 0;
					largest = IRect(startX, startY, startX + rectW, startY + rectH);
				}
			}
			stack[++top] = y;
		}
	}
}

static inline VoxelVertex makeV(const glm::vec3 &p) {
	VoxelVertex v;
	v.position = p;
	v.info = 0;
	v.colorIndex = 0;
	v.normalIndex = 0;
	v.padding2 = 0;
	return v;
}

static void extractFace(SurfaceExtractionContext &ctx, TexturSurfaceMesherState &state, FaceNames face) {
	const Region &region = ctx.region;
	const RawVolume *volume = ctx.volume;
	const palette::Palette &palette = ctx.palette;

	int uDim = 0, vDim = 0, sDim = 0;
	// Map axes based on face
	const int rx = region.getLowerX();
	const int ry = region.getLowerY();
	const int rz = region.getLowerZ();
	const int rw = region.getWidthInVoxels();
	const int rh = region.getHeightInVoxels();
	const int rd = region.getDepthInVoxels();

	glm::ivec3 axes;

	switch (face) {
	case FaceNames::Left:
	case FaceNames::Right:
		sDim = rw;
		uDim = rh;
		vDim = rd;
		axes = glm::ivec3(1, 2, 0);
		break;
	case FaceNames::Down:
	case FaceNames::Up:
		sDim = rh;
		uDim = rw;
		vDim = rd;
		axes = glm::ivec3(0, 2, 1);
		break;
	case FaceNames::Front:
	case FaceNames::Back:
		sDim = rd;
		uDim = rw;
		vDim = rh;
		axes = glm::ivec3(0, 1, 2);
		break;
	default:
		return;
	}

	core::Buffer<color::RGBA> mask;
	mask.resize(uDim * vDim);

	// Pre-compute face-invariant values
	voxel::Mesh &mesh = ctx.mesh.mesh[0];
	VertexArray &vertices = mesh.getVertexVector();
	UVArray &uvs = mesh.getUVVector();
	IndexArray &indices = mesh.getIndexVector();
	NormalArray &normals = mesh.getNormalVector();

	const float invTex = 1.0f / (float)TexturSurfaceMesherState::TEX_SIZE;
	const bool flip = isPositiveFace(face);

	glm::vec3 n;
	if (face == FaceNames::Left) {
		n = glm::right();
	} else if (face == FaceNames::Right) {
		n = glm::left();
	} else if (face == FaceNames::Down) {
		n = glm::down();
	} else if (face == FaceNames::Up) {
		n = glm::up();
	} else if (face == FaceNames::Front) {
		n = glm::backward();
	} else if (face == FaceNames::Back) {
		n = glm::forward();
	}

	core::Buffer<int> heights;
	heights.resize(vDim);
	core::Buffer<int> rectStack;
	rectStack.resize(vDim + 1);

	for (int s = 0; s < sDim; ++s) {
		mask.fill(color::RGBA(0));

		int voxelS = s;

		// Fill mask
		for (int u = 0; u < uDim; ++u) {
			for (int v = 0; v < vDim; ++v) {
				glm::ivec3 pos;
				pos[axes.x] = u;
				pos[axes.y] = v;
				pos[axes.z] = voxelS;

				glm::ivec3 absPos(rx + pos.x, ry + pos.y, rz + pos.z);

				const Voxel &vox = volume->voxel(absPos);
				if (isBlocked(vox.getMaterial())) {
					glm::ivec3 neighborPos = absPos;
					if (isPositiveFace(face)) {
						neighborPos[axes.z]++;
					} else {
						neighborPos[axes.z]--;
					}

					const Voxel &neighbor = volume->voxel(neighborPos);
					if (!isBlocked(neighbor.getMaterial())) {
						mask[u * vDim + v] = palette.color(vox.getColor());
					}
				}
			}
		}

		// Greedy mesh the mask
		const int sOffset = flip ? s + 1 : s;
		while (true) {
			IRect largest;
			findLargestRect(mask, uDim, vDim, heights, rectStack, largest);

			if (largest.width() == 0) {
				break;
			}

			IRect uvRect = state.add(mask, vDim, largest);
			if (uvRect.width() <= 0) {
				break;
			}

			const int u1 = largest.getMinX();
			const int v1 = largest.getMinZ();
			const int u2 = largest.getMaxX();
			const int v2 = largest.getMaxZ();

			const int idx = (int)mesh.getNoOfVertices();

			const float tu1 = uvRect.getMinX() * invTex;
			const float tv1 = uvRect.getMinZ() * invTex;
			const float tu2 = uvRect.getMaxX() * invTex;
			const float tv2 = uvRect.getMaxZ() * invTex;

			const glm::vec2 tex[]{glm::vec2(tu1, tv1), glm::vec2(tu2, tv1), glm::vec2(tu2, tv2), glm::vec2(tu1, tv2)};

			auto makeVert = [&](int u, int v) {
				glm::ivec3 pos;
				pos[axes.x] = u;
				pos[axes.y] = v;
				pos[axes.z] = sOffset;
				return glm::vec3(pos);
			};

			const glm::vec3 p0 = makeVert(u1, v1);
			const glm::vec3 p1 = makeVert(u2, v1);
			const glm::vec3 p2 = makeVert(u2, v2);
			const glm::vec3 p3 = makeVert(u1, v2);

			if (flip) {
				vertices.emplace_back(makeV(p0));
				vertices.emplace_back(makeV(p3));
				vertices.emplace_back(makeV(p2));
				vertices.emplace_back(makeV(p1));

				uvs.emplace_back(tex[0]);
				uvs.emplace_back(tex[3]);
				uvs.emplace_back(tex[2]);
				uvs.emplace_back(tex[1]);
			} else {
				vertices.emplace_back(makeV(p0));
				vertices.emplace_back(makeV(p1));
				vertices.emplace_back(makeV(p2));
				vertices.emplace_back(makeV(p3));

				uvs.emplace_back(tex[0]);
				uvs.emplace_back(tex[1]);
				uvs.emplace_back(tex[2]);
				uvs.emplace_back(tex[3]);
			}

			indices.emplace_back(idx + 0);
			indices.emplace_back(idx + 1);
			indices.emplace_back(idx + 2);

			indices.emplace_back(idx + 0);
			indices.emplace_back(idx + 2);
			indices.emplace_back(idx + 3);

			normals.emplace_back(n);
			normals.emplace_back(n);
			normals.emplace_back(n);
			normals.emplace_back(n);

			for (int cx = largest.getMinX(); cx < largest.getMaxX(); ++cx) {
				core_memset(&mask[cx * vDim + largest.getMinZ()], 0, largest.height() * sizeof(color::RGBA));
			}
		}
	}
}

} // namespace

void extractTextureMesh(SurfaceExtractionContext &ctx) {
	TexturSurfaceMesherState state;

	extractFace(ctx, state, FaceNames::Left);
	extractFace(ctx, state, FaceNames::Right);
	extractFace(ctx, state, FaceNames::Down);
	extractFace(ctx, state, FaceNames::Up);
	extractFace(ctx, state, FaceNames::Front);
	extractFace(ctx, state, FaceNames::Back);

	ctx.textureWidth = state.rect.getMaxX() + 1;
	ctx.textureHeight = state.rect.getMaxZ() + 1;

	if (ctx.textureWidth <= 0) {
		ctx.textureWidth = 1;
	}
	if (ctx.textureHeight <= 0) {
		ctx.textureHeight = 1;
	}

	const float sU = (float)TexturSurfaceMesherState::TEX_SIZE / (float)ctx.textureWidth;
	const float sV = (float)TexturSurfaceMesherState::TEX_SIZE / (float)ctx.textureHeight;

	voxel::Mesh &mesh = ctx.mesh.mesh[0];
	voxel::UVArray &uvs = mesh.getUVVector();
	for (glm::vec2 &uv : uvs) {
		uv.x *= sU;
		uv.y *= sV;
	}

	ctx.textureData.resize(ctx.textureWidth * ctx.textureHeight * 4);

	for (int y = 0; y < ctx.textureHeight; ++y) {
		for (int x = 0; x < ctx.textureWidth; ++x) {
			color::RGBA rgba = state.colors[y * TexturSurfaceMesherState::TEX_SIZE + x];
			if (rgba.a == 0) {
				continue;
			}
			const int baseIdx = (y * ctx.textureWidth + x) * 4;
			ctx.textureData[baseIdx + 0] = rgba.r;
			ctx.textureData[baseIdx + 1] = rgba.g;
			ctx.textureData[baseIdx + 2] = rgba.b;
			ctx.textureData[baseIdx + 3] = rgba.a;
		}
	}
}

} // namespace voxel
