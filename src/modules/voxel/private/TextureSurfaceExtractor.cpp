/**
 * @file
 */

#include "TextureSurfaceExtractor.h"
#include "core/Common.h"
#include "core/Hash.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicMap.h"
#include "palette/Palette.h"
#include "voxel/ChunkMesh.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/VoxelVertex.h"
#include "voxel/external/stb_rect_pack.h"
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace voxel {

namespace {

static constexpr int PADDING = 1;

struct PendingQuad {
	int u1, v1, u2, v2;
	int sOffset;
	int axesX, axesY, axesZ;
	glm::vec3 normal;
	bool flip;
	int packIndex;
	int colorW, colorH;
};

using DedupeMap = core::DynamicMap<uint32_t, int, 1021>;

static inline VoxelVertex makeV(const glm::vec3 &p) {
	VoxelVertex v;
	v.position = p;
	v.info = 0;
	v.colorIndex = 0;
	v.normalIndex = 0;
	v.padding2 = 0;
	return v;
}

// Extract greedy rectangles from mask, save per-rect colors, record quads and pack rects
static void greedyExtract(color::RGBA *mask, int uDim, int vDim, int sOffset, const glm::ivec3 &axes,
						  const glm::vec3 &normal, bool flip, core::Buffer<PendingQuad> &quads,
						  core::Buffer<color::RGBA> &allColors, core::Buffer<stbrp_rect> &packRects,
						  core::Buffer<int> &packColorOffsets, DedupeMap *dedupeMap) {
	for (int u = 0; u < uDim; ++u) {
		int v = 0;
		while (v < vDim) {
			const int maskColBase = u * vDim;
			if (mask[maskColBase + v].a == 0) {
				++v;
				continue;
			}

			// Extend in v direction (contiguous in memory)
			int spanH = 1;
			while (v + spanH < vDim && mask[maskColBase + v + spanH].a != 0) {
				++spanH;
			}

			// Extend in u direction
			int spanW = 1;
			while (u + spanW < uDim) {
				const int base = (u + spanW) * vDim + v;
				bool full = true;
				for (int dy = 0; dy < spanH; ++dy) {
					if (mask[base + dy].a == 0) {
						full = false;
						break;
					}
				}
				if (!full) {
					break;
				}
				++spanW;
			}

			const int w = spanW;
			const int h = spanH;

			// Save colors from mask to allColors (row-major for efficient atlas copy)
			const int colorOffset = (int)allColors.size();
			allColors.resize(allColors.size() + w * h);
			color::RGBA *dst = &allColors[colorOffset];
			for (int gy = 0; gy < h; ++gy) {
				for (int gx = 0; gx < w; ++gx) {
					dst[gy * w + gx] = mask[(u + gx) * vDim + (v + gy)];
				}
			}

			int packIndex = (int)packRects.size();
			bool reused = false;
			if (dedupeMap != nullptr) {
				const uint32_t seed = ((uint32_t)w << 16) ^ (uint32_t)h;
				const uint32_t colorHash =
					core::hash(dst, w * h * (int)sizeof(color::RGBA), seed);
				int existing = -1;
				if (dedupeMap->get(colorHash, existing)) {
					const stbrp_rect &existingRect = packRects[existing];
					if (existingRect.w == w + PADDING && existingRect.h == h + PADDING) {
						const color::RGBA *existingColors = &allColors[packColorOffsets[existing]];
						if (core_memcmp(existingColors, dst, (size_t)w * h * sizeof(color::RGBA)) == 0) {
							packIndex = existing;
							allColors.resize(colorOffset);
							reused = true;
						}
					}
				}
				if (!reused) {
					dedupeMap->putIfAbsent(colorHash, packIndex);
				}
			}

			PendingQuad q;
			q.u1 = u;
			q.v1 = v;
			q.u2 = u + w;
			q.v2 = v + h;
			q.sOffset = sOffset;
			q.axesX = axes.x;
			q.axesY = axes.y;
			q.axesZ = axes.z;
			q.normal = normal;
			q.flip = flip;
			q.packIndex = packIndex;
			q.colorW = w;
			q.colorH = h;
			quads.push_back(q);

			if (!reused) {
				stbrp_rect r;
				r.id = packIndex;
				r.w = w + PADDING;
				r.h = h + PADDING;
				r.was_packed = 0;
				r.x = 0;
				r.y = 0;
				packRects.push_back(r);
				packColorOffsets.push_back(colorOffset);
			}

			// Clear mask region
			for (int cx = u; cx < u + w; ++cx) {
				const int cxBase = cx * vDim + v;
				for (int cy = 0; cy < h; ++cy) {
					mask[cxBase + cy] = color::RGBA(0);
				}
			}

			v += spanH;
		}
	}
}

// Process a pair of opposite faces (positive + negative) sharing the same axis in a single pass
static void collectFacePair(SurfaceExtractionContext &ctx, core::Buffer<PendingQuad> &quads,
							core::Buffer<color::RGBA> &allColors, core::Buffer<stbrp_rect> &packRects,
							core::Buffer<int> &packColorOffsets, DedupeMap *dedupeMap, FaceNames posFace,
							FaceNames negFace, color::RGBA *maskPos, color::RGBA *maskNeg) {
	const Region &region = ctx.region;
	const RawVolume *volume = ctx.volume;

	int uDim = 0, vDim = 0, sDim = 0;
	const int rx = region.getLowerX();
	const int ry = region.getLowerY();
	const int rz = region.getLowerZ();
	const int rw = region.getWidthInVoxels();
	const int rh = region.getHeightInVoxels();
	const int rd = region.getDepthInVoxels();

	glm::ivec3 axes;

	switch (posFace) {
	case FaceNames::Right:
		sDim = rw;
		uDim = rh;
		vDim = rd;
		axes = glm::ivec3(1, 2, 0);
		break;
	case FaceNames::Up:
		sDim = rh;
		uDim = rw;
		vDim = rd;
		axes = glm::ivec3(0, 2, 1);
		break;
	case FaceNames::Back:
		sDim = rd;
		uDim = rw;
		vDim = rh;
		axes = glm::ivec3(0, 1, 2);
		break;
	default:
		return;
	}

	// Direct volume access with precomputed strides
	const Voxel *volData = volume->voxels();
	const Region &volRegion = volume->region();
	const int64_t volW = volume->width();
	const int64_t volStride = volRegion.stride();

	const int64_t dimStrides[3] = {1, volW, volStride};
	const int64_t uVolStride = dimStrides[axes.x];
	const int64_t vVolStride = dimStrides[axes.y];
	const int64_t sVolStride = dimStrides[axes.z];

	const int64_t volBase =
		(rx - volRegion.getLowerX()) + (int64_t)(ry - volRegion.getLowerY()) * volW + (int64_t)(rz - volRegion.getLowerZ()) * volStride;

	const int rOrigin[3] = {rx, ry, rz};
	const int sAbsBase = rOrigin[axes.z];
	const int volLowerArr[3] = {volRegion.getLowerX(), volRegion.getLowerY(), volRegion.getLowerZ()};
	const int volUpperArr[3] = {volRegion.getUpperX(), volRegion.getUpperY(), volRegion.getUpperZ()};
	const int sVolLower = volLowerArr[axes.z];
	const int sVolUpper = volUpperArr[axes.z];

	// Clamp iteration to volume bounds (extraction region may extend beyond volume)
	const int sIterMax = core_min(sDim, volUpperArr[axes.z] - sAbsBase + 1);
	const int uIterMax = core_min(uDim, volUpperArr[axes.x] - rOrigin[axes.x] + 1);
	const int vIterMax = core_min(vDim, volUpperArr[axes.y] - rOrigin[axes.y] + 1);

	const glm::vec3 nPos = faceNormal(posFace);
	const glm::vec3 nNeg = faceNormal(negFace);
	// The (u,v) axes for Y faces form a left-handed pair (X,Z), so the
	// non-flipped cross product points -Y. For X and Z faces the pair is
	// right-handed and the cross points in the positive axis direction.
	// Flip the winding whenever the natural cross disagrees with the face normal.
	const bool flipPos = isNegativeFace(posFace) != isY(posFace);
	const bool flipNeg = isNegativeFace(negFace) != isY(negFace);

	for (int s = 0; s < sIterMax; ++s) {
		const int sAbs = sAbsBase + s;
		const int neighborSPos = sAbs + 1;
		const int neighborSNeg = sAbs - 1;
		const bool posNeighborInVolume = (neighborSPos >= sVolLower && neighborSPos <= sVolUpper);
		const bool negNeighborInVolume = (neighborSNeg >= sVolLower && neighborSNeg <= sVolUpper);

		const int64_t sliceBase = volBase + s * sVolStride;

		int maskCountPos = 0, maskCountNeg = 0;
		for (int u = 0; u < uIterMax; ++u) {
			int64_t volIdx = sliceBase + u * uVolStride;
			const int maskColBase = u * vDim;
			for (int v = 0; v < vIterMax; ++v) {
				const Voxel &vox = volData[volIdx];
				if (isBlocked(vox.getMaterial())) {
					const color::RGBA color = ctx.palette.color(vox.getColor());

					// Positive face (neighbor at s+1)
					if (posNeighborInVolume) {
						if (!isBlocked(volData[volIdx + sVolStride].getMaterial())) {
							maskPos[maskColBase + v] = color;
							++maskCountPos;
						}
					} else {
						maskPos[maskColBase + v] = color;
						++maskCountPos;
					}

					// Negative face (neighbor at s-1)
					if (negNeighborInVolume) {
						if (!isBlocked(volData[volIdx - sVolStride].getMaterial())) {
							maskNeg[maskColBase + v] = color;
							++maskCountNeg;
						}
					} else {
						maskNeg[maskColBase + v] = color;
						++maskCountNeg;
					}
				}
				volIdx += vVolStride;
			}
		}

		if (maskCountPos > 0) {
			greedyExtract(maskPos, uDim, vDim, s + 1, axes, nPos, flipPos, quads, allColors, packRects,
						  packColorOffsets, dedupeMap);
		}
		if (maskCountNeg > 0) {
			greedyExtract(maskNeg, uDim, vDim, s, axes, nNeg, flipNeg, quads, allColors, packRects,
						  packColorOffsets, dedupeMap);
		}
	}
}

} // namespace

void extractTextureMesh(SurfaceExtractionContext &ctx) {
	const Region &region = ctx.region;
	const int rw = region.getWidthInVoxels();
	const int rh = region.getHeightInVoxels();
	const int rd = region.getDepthInVoxels();

	// Upper bound: every voxel can expose all 6 faces (checkerboard / sparse
	// volumes). Surface-area estimates under-count heavily and cause realloc churn.
	const int maxExposed = rw * rh * rd * 6;
	const int needed = maxExposed + maxExposed / 2;
	int texSize = 64;
	while (texSize * texSize < needed && texSize < 2048) {
		texSize *= 2;
	}

	// Shared mask buffers for face pair processing (zeroed by constructor)
	const int maxMask = core_max(rh * rd, core_max(rw * rd, rw * rh));
	core::Buffer<color::RGBA> maskPos(maxMask);
	core::Buffer<color::RGBA> maskNeg(maxMask);

	// Collection buffers for deferred batch packing
	core::Buffer<PendingQuad> quads;
	core::Buffer<color::RGBA> allColors;
	core::Buffer<stbrp_rect> packRects;
	core::Buffer<int> packColorOffsets;
	quads.reserve(maxExposed);
	allColors.reserve(maxExposed);
	packRects.reserve(maxExposed);
	packColorOffsets.reserve(maxExposed);

	DedupeMap dedupeMapStorage;
	DedupeMap *dedupeMap = nullptr;
	if (ctx.textureDedupe) {
		dedupeMap = &dedupeMapStorage;
	}

	// Process face pairs - each pair reads the volume once for both faces
	collectFacePair(ctx, quads, allColors, packRects, packColorOffsets, dedupeMap, FaceNames::Right,
					FaceNames::Left, maskPos.data(), maskNeg.data());
	collectFacePair(ctx, quads, allColors, packRects, packColorOffsets, dedupeMap, FaceNames::Up, FaceNames::Down,
					maskPos.data(), maskNeg.data());
	collectFacePair(ctx, quads, allColors, packRects, packColorOffsets, dedupeMap, FaceNames::Back,
					FaceNames::Front, maskPos.data(), maskNeg.data());

	if (quads.empty()) {
		ctx.textureWidth = 1;
		ctx.textureHeight = 1;
		ctx.textureData.resize(4);
		return;
	}

	if (dedupeMap != nullptr && packRects.size() < quads.size()) {
		Log::debug("Texture atlas dedupe: %i unique of %i quads", (int)packRects.size(), (int)quads.size());
	}

	// Batch pack all rectangles; grow the atlas if packing fails (padding makes the
	// surface-area estimate optimistic and can leave many quads unpacked).
	int unpacked = 0;
	for (;;) {
		core::Buffer<stbrp_node> nodes(texSize);
		stbrp_context stbCtx;
		stbrp_init_target(&stbCtx, texSize, texSize, nodes.data(), texSize);
		// Reset packing state - stbrp_pack_rects overwrites x/y/was_packed
		for (size_t i = 0; i < packRects.size(); ++i) {
			packRects[i].was_packed = 0;
			packRects[i].x = 0;
			packRects[i].y = 0;
		}
		stbrp_pack_rects(&stbCtx, packRects.data(), (int)packRects.size());

		unpacked = 0;
		for (size_t i = 0; i < packRects.size(); ++i) {
			if (!packRects[i].was_packed) {
				++unpacked;
			}
		}
		if (unpacked == 0 || texSize >= 4096) {
			break;
		}
		texSize *= 2;
		Log::debug("Texture atlas pack incomplete (%i unpacked) - retrying at %i", unpacked, texSize);
	}

	// Determine final texture bounds from unique packed rects
	int boundX = 0, boundY = 0;
	for (size_t i = 0; i < packRects.size(); ++i) {
		const stbrp_rect &r = packRects[i];
		if (r.was_packed) {
			const int ex = r.x + (r.w - PADDING);
			const int ey = r.y + (r.h - PADDING);
			if (ex > boundX) {
				boundX = ex;
			}
			if (ey > boundY) {
				boundY = ey;
			}
		}
	}

	ctx.textureWidth = boundX + 1;
	ctx.textureHeight = boundY + 1;
	if (ctx.textureWidth <= 0) {
		ctx.textureWidth = 1;
	}
	if (ctx.textureHeight <= 0) {
		ctx.textureHeight = 1;
	}

	// Allocate texture and emit geometry in a single pass over collected quads
	ctx.textureData.resize(ctx.textureWidth * ctx.textureHeight * 4);

	const float invTexW = 1.0f / (float)ctx.textureWidth;
	const float invTexH = 1.0f / (float)ctx.textureHeight;

	voxel::Mesh &mesh = ctx.mesh.mesh[0];
	VertexArray &vertices = mesh.getVertexVector();
	UVArray &uvs = mesh.getUVVector();
	IndexArray &indices = mesh.getIndexVector();
	NormalArray &normals = mesh.getNormalVector();

	const size_t quadCount = quads.size();
	vertices.reserve(quadCount * 4);
	uvs.reserve(quadCount * 4);
	indices.reserve(quadCount * 6);
	normals.reserve(quadCount * 4);

	// Copy each unique packed patch once
	for (size_t i = 0; i < packRects.size(); ++i) {
		const stbrp_rect &r = packRects[i];
		if (!r.was_packed) {
			continue;
		}
		const int w = r.w - PADDING;
		const int h = r.h - PADDING;
		const color::RGBA *src = &allColors[packColorOffsets[i]];
		for (int row = 0; row < h; ++row) {
			const int dstOffset = ((r.y + row) * ctx.textureWidth + r.x) * 4;
			core_memcpy(&ctx.textureData[dstOffset], &src[row * w], (size_t)w * 4);
		}
	}

	int stillUnpacked = 0;
	for (size_t i = 0; i < quadCount; ++i) {
		const PendingQuad &q = quads[i];
		const stbrp_rect &r = packRects[q.packIndex];

		if (!r.was_packed) {
			++stillUnpacked;
			continue;
		}

		const int px = r.x;
		const int py = r.y;
		const int w = q.colorW;
		const int h = q.colorH;

		// Compute UVs directly in final texture space (no rescaling pass needed)
		const float tu1 = (float)px * invTexW;
		const float tv1 = (float)py * invTexH;
		const float tu2 = (float)(px + w) * invTexW;
		const float tv2 = (float)(py + h) * invTexH;

		const glm::vec2 tex[]{glm::vec2(tu1, tv1), glm::vec2(tu2, tv1), glm::vec2(tu2, tv2), glm::vec2(tu1, tv2)};

		auto makeVert = [&](int mu, int mv) {
			glm::ivec3 pos;
			pos[q.axesX] = mu;
			pos[q.axesY] = mv;
			pos[q.axesZ] = q.sOffset;
			return glm::vec3(pos);
		};

		const glm::vec3 p0 = makeVert(q.u1, q.v1);
		const glm::vec3 p1 = makeVert(q.u2, q.v1);
		const glm::vec3 p2 = makeVert(q.u2, q.v2);
		const glm::vec3 p3 = makeVert(q.u1, q.v2);

		const int idx = (int)mesh.getNoOfVertices();

		if (q.flip) {
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

		normals.emplace_back(q.normal);
		normals.emplace_back(q.normal);
		normals.emplace_back(q.normal);
		normals.emplace_back(q.normal);
	}
	if (stillUnpacked > 0) {
		Log::warn("Texture atlas full: dropped %i of %i quads (atlas %ix%i)", stillUnpacked, (int)quadCount,
				  ctx.textureWidth, ctx.textureHeight);
	}
}

} // namespace voxel
