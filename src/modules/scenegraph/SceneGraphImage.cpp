/**
 * @file
 */

#include "SceneGraphImage.h"
#include "SceneGraph.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/collection/Buffer.h"
#include "voxel/RawVolume.h"
#include "voxel/SparseVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxelutil/VoxelUtil.h"

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

void bakeNode(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
			  scenegraph::FrameIndex frameIdx, palette::Palette &palette, voxel::SparseVolume &target) {
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
		target.setVoxel(x, y, z, voxel::createVoxel(palette, index));
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
};

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
		out.volume = tightVolume(volume, out.cropped);
		return out.volume != nullptr;
	}

	voxel::SparseVolume sparse;
	for (int id : ids) {
		bakeNode(sceneGraph, sceneGraph.node(id), frameIdx, out.palette, sparse);
	}
	const voxel::Region region = sparse.calculateRegion();
	if (!region.isValid()) {
		return false;
	}
	out.owned = new voxel::RawVolume(region);
	if (out.owned->voxels() == nullptr) {
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
	return voxelutil::renderIsometricImage(baked.volume, baked.palette, config);
}

} // namespace scenegraph
