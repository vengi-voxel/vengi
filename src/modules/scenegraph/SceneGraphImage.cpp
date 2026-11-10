/**
 * @file
 */

#include "SceneGraphImage.h"
#include "SceneGraph.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/collection/Buffer.h"
#include "palette/NormalPalette.h"
#include "voxel/RawVolume.h"
#include "voxel/SparseVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxelutil/VoxelUtil.h"
#include <glm/exponential.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>

namespace scenegraph {
namespace {

void collectModelNodes(const scenegraph::SceneGraph &sceneGraph, int nodeId, core::Buffer<int> &ids) {
	auto add = [&](const scenegraph::SceneGraphNode &node) {
		if (!node.isAnyModelNode() || !node.visible()) {
			return;
		}
		const voxel::RawVolume *volume = sceneGraph.resolveVolume(node);
		if (volume == nullptr || !volume->region().isValid()) {
			return;
		}
		ids.push_back(node.id());
	};
	if (nodeId == InvalidNodeId) {
		sceneGraph.visitChildren(sceneGraph.root().id(), true, add);
		return;
	}
	if (!sceneGraph.hasNode(nodeId)) {
		return;
	}
	add(sceneGraph.node(nodeId));
	sceneGraph.visitChildren(nodeId, true, add);
}

// Keep the alpha channel. A full 256-color node palette must not crowd out a later
// model's glass color, and an RGB-only match would turn that glass opaque.
uint8_t takeColor(palette::Palette &palette, color::RGBA rgba, const palette::Material &mat) {
	uint8_t index = 0;
	if (palette.tryAdd(rgba, false, &index, false, -1)) {
		palette.setMaterial(index, mat);
		return index;
	}
	if (palette.hasColor(rgba)) {
		return index;
	}
	int best = 0;
	int bestDist = 1 << 30;
	for (int i = 0; i < palette.colorCount(); ++i) {
		const color::RGBA c = palette.color(i);
		const int dr = (int)c.r - (int)rgba.r;
		const int dg = (int)c.g - (int)rgba.g;
		const int db = (int)c.b - (int)rgba.b;
		const int da = (int)c.a - (int)rgba.a;
		const int dist = dr * dr + dg * dg + db * db + da * da;
		if (dist < bestDist) {
			bestDist = dist;
			best = i;
		}
	}
	return (uint8_t)best;
}

glm::mat3 normalMatrixFromWorld(const glm::mat4 &world) {
	const glm::mat3 linear(world);
	const float det = glm::determinant(linear);
	if (det > -1.0e-8f && det < 1.0e-8f) {
		return glm::mat3(1.0f);
	}
	return glm::transpose(glm::inverse(linear));
}

bool isIdentityMat3(const glm::mat3 &m) {
	return glm::all(glm::epsilonEqual(m[0], glm::vec3(1.0f, 0.0f, 0.0f), 0.001f)) &&
		   glm::all(glm::epsilonEqual(m[1], glm::vec3(0.0f, 1.0f, 0.0f), 0.001f)) &&
		   glm::all(glm::epsilonEqual(m[2], glm::vec3(0.0f, 0.0f, 1.0f), 0.001f));
}

// Reuse a palette slot when the direction is already stored. Otherwise append it.
int takeNormal(palette::NormalPalette &dst, const glm::vec3 &direction) {
	const float len2 = glm::dot(direction, direction);
	if (len2 <= 1.0e-8f) {
		return palette::PaletteNormalNotFound;
	}
	const glm::vec3 n = direction * (1.0f / glm::sqrt(len2));
	int match = palette::PaletteNormalNotFound;
	if (dst.size() > 0) {
		match = dst.getClosestMatch(n);
		if (match != palette::PaletteNormalNotFound) {
			const glm::vec3 existing = dst.normal3f((uint8_t)match);
			const float existingLen2 = glm::dot(existing, existing);
			if (existingLen2 > 1.0e-8f &&
				glm::dot(existing * (1.0f / glm::sqrt(existingLen2)), n) >= 0.99f) {
				return match;
			}
		}
	}
	if ((int)dst.size() >= palette::NormalPaletteMaxNormals) {
		return match;
	}
	const int index = (int)dst.size();
	dst.setNormal((uint8_t)index, n);
	return index;
}

// Remap values are voxel normal indices (palette slot + NORMAL_PALETTE_OFFSET).
void buildNormalRemap(const palette::NormalPalette &src, const glm::mat3 &normalMat, palette::NormalPalette &dst,
					  uint8_t *remap) {
	const bool identity = isIdentityMat3(normalMat);
	for (int i = 0; i < palette::NormalPaletteMaxNormals; ++i) {
		remap[i] = NO_NORMAL;
		if (i >= (int)src.size() || src.normal((uint8_t)i).a == 0) {
			continue;
		}
		if (identity && i < (int)dst.size() && src.normal((uint8_t)i) == dst.normal((uint8_t)i)) {
			remap[i] = (uint8_t)(i + NORMAL_PALETTE_OFFSET);
			continue;
		}
		glm::vec3 direction = src.normal3f((uint8_t)i);
		const float len2 = glm::dot(direction, direction);
		if (len2 <= 1.0e-8f) {
			continue;
		}
		if (!identity) {
			direction = normalMat * direction;
		}
		const int match = takeNormal(dst, direction);
		if (match >= 0) {
			remap[i] = (uint8_t)(match + NORMAL_PALETTE_OFFSET);
		}
	}
}

uint8_t mapNormalIndex(uint8_t normalIndex, const uint8_t *remap) {
	if (remap == nullptr || normalIndex == NO_NORMAL) {
		return NO_NORMAL;
	}
	const int idx = (int)normalIndex - NORMAL_PALETTE_OFFSET;
	if (idx < 0 || idx >= palette::NormalPaletteMaxNormals) {
		return NO_NORMAL;
	}
	return remap[idx];
}

void remapVolumeNormals(voxel::RawVolume &volume, const uint8_t *remap) {
	voxelutil::visitVolume(volume, [&volume, remap](int x, int y, int z, const voxel::Voxel &voxel) {
		const uint8_t mapped = mapNormalIndex(voxel.getNormal(), remap);
		if (mapped == voxel.getNormal()) {
			return;
		}
		voxel::Voxel updated = voxel;
		updated.setNormal(mapped);
		volume.setVoxel(x, y, z, updated);
	});
}

void bakeNode(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
			  scenegraph::FrameIndex frameIdx, palette::Palette &palette, voxel::SparseVolume &target,
			  const uint8_t *normalRemap) {
	const voxel::RawVolume *volume = sceneGraph.resolveVolume(node);
	if (volume == nullptr || !volume->region().isValid()) {
		return;
	}
	const palette::Palette &nodePalette = sceneGraph.resolvePalette(node);
	auto writeVoxel = [&](int x, int y, int z, const voxel::Voxel &voxel) {
		if (voxel::isAir(voxel.getMaterial())) {
			return;
		}
		const uint8_t src = voxel.getColor();
		const uint8_t index = takeColor(palette, nodePalette.color(src), nodePalette.material(src));
		const uint8_t normal = mapNormalIndex(voxel.getNormal(), normalRemap);
		target.setVoxel(x, y, z, voxel::createVoxel(palette, index, normal));
	};

	const scenegraph::FrameTransform &transform = sceneGraph.transformForFrame(node, frameIdx);
	if (transform.isIdentity()) {
		voxelutil::visitVolume(*volume, writeVoxel);
		return;
	}
	core::ScopedPtr<voxel::RawVolume> baked(
		voxelutil::applyTransformToVolume(*volume, transform.worldMatrix(), node.pivot()));
	if (baked == nullptr) {
		return;
	}
	voxelutil::visitVolume(*baked, writeVoxel);
}

const voxel::RawVolume *tightVolume(const voxel::RawVolume *volume, core::ScopedPtr<voxel::RawVolume> &cropped) {
	glm::ivec3 mins;
	glm::ivec3 maxs;
	if (!voxelutil::computeSolidBounds(volume, mins, maxs)) {
		return nullptr;
	}
	if (mins == volume->region().getLowerCorner() && maxs == volume->region().getUpperCorner()) {
		return volume;
	}
	cropped = voxelutil::cropVolume(volume, mins, maxs);
	return cropped != nullptr ? (const voxel::RawVolume *)cropped : volume;
}

struct BakedScene {
	core::ScopedPtr<voxel::RawVolume> owned;
	core::ScopedPtr<voxel::RawVolume> cropped;
	const voxel::RawVolume *volume = nullptr;
	palette::Palette palette;
	palette::NormalPalette normalPalette;
	bool useNormals = false;
};

const scenegraph::SceneGraphNode *findNormalPaletteNode(const scenegraph::SceneGraph &sceneGraph,
														const core::Buffer<int> &ids) {
	for (int id : ids) {
		const scenegraph::SceneGraphNode &node = sceneGraph.node(id);
		if (node.hasNormalPalette() && node.normalPalette().size() > 0) {
			return &node;
		}
	}
	return nullptr;
}

bool bakeScene(const scenegraph::SceneGraph &sceneGraph, int nodeId, scenegraph::FrameIndex frameIdx, BakedScene &out) {
	core::Buffer<int> ids;
	collectModelNodes(sceneGraph, nodeId, ids);
	if (ids.empty()) {
		return false;
	}
	if (ids.size() == 1u) {
		out.palette = sceneGraph.resolvePalette(sceneGraph.node(ids[0]));
		const scenegraph::SceneGraphNode &node = sceneGraph.node(ids[0]);
		const voxel::RawVolume *volume = sceneGraph.resolveVolume(node);
		const scenegraph::FrameTransform &transform = sceneGraph.transformForFrame(node, frameIdx);
		if (!transform.isIdentity()) {
			out.owned = voxelutil::applyTransformToVolume(*volume, transform.worldMatrix(), node.pivot());
			if (out.owned == nullptr) {
				return false;
			}
			volume = out.owned;
		}
		if (node.hasNormalPalette() && node.normalPalette().size() > 0) {
			out.normalPalette = node.normalPalette();
			out.useNormals = true;
			const glm::mat3 normalMat = normalMatrixFromWorld(transform.worldMatrix());
			if (out.owned != nullptr && !isIdentityMat3(normalMat)) {
				uint8_t remap[palette::NormalPaletteMaxNormals];
				buildNormalRemap(node.normalPalette(), normalMat, out.normalPalette, remap);
				remapVolumeNormals(*out.owned, remap);
				volume = out.owned;
			}
		}
		out.volume = tightVolume(volume, out.cropped);
		return out.volume != nullptr;
	}

	if (const scenegraph::SceneGraphNode *normalNode = findNormalPaletteNode(sceneGraph, ids)) {
		out.normalPalette = normalNode->normalPalette();
		out.useNormals = true;
	}

	voxel::SparseVolume sparse;
	for (int id : ids) {
		const scenegraph::SceneGraphNode &node = sceneGraph.node(id);
		uint8_t remap[palette::NormalPaletteMaxNormals];
		const uint8_t *remapPtr = nullptr;
		if (out.useNormals && node.hasNormalPalette() && node.normalPalette().size() > 0) {
			const glm::mat3 normalMat =
				normalMatrixFromWorld(sceneGraph.transformForFrame(node, frameIdx).worldMatrix());
			buildNormalRemap(node.normalPalette(), normalMat, out.normalPalette, remap);
			remapPtr = remap;
		}
		bakeNode(sceneGraph, node, frameIdx, out.palette, sparse, remapPtr);
	}
	const voxel::Region region = sparse.calculateRegion();
	if (!region.isValid()) {
		return false;
	}
	out.owned = new voxel::RawVolume(region);
	if (!out.owned->valid()) {
		Log::error("Failed to allocate scene image volume");
		return false;
	}
	sparse.copyTo(*out.owned);
	out.volume = out.owned;
	return true;
}

} // namespace

image::ImagePtr renderToImage(const SceneGraph &sceneGraph, const voxelutil::ImageRenderConfig &config, int nodeId,
							  FrameIndex frameIdx) {
	BakedScene baked;
	if (!bakeScene(sceneGraph, nodeId, frameIdx, baked)) {
		return {};
	}
	return voxelutil::renderToImage(baked.volume, baked.palette, config);
}

image::ImagePtr renderIsometricImage(const SceneGraph &sceneGraph, const voxelutil::IsoRenderConfig &config,
									 int nodeId, FrameIndex frameIdx) {
	BakedScene baked;
	if (!bakeScene(sceneGraph, nodeId, frameIdx, baked)) {
		return {};
	}
	const palette::NormalPalette *normals = baked.useNormals ? &baked.normalPalette : nullptr;
	return voxelutil::renderIsometricImage(baked.volume, baked.palette, config, normals);
}

} // namespace scenegraph
