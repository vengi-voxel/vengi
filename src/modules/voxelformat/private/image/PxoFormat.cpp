/**
 * @file
 */

#include "PxoFormat.h"
#include "core/Common.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicArray.h"
#include "image/Image.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryArchive.h"
#include "io/Stream.h"
#include "io/ZipArchive.h"
#include "json/JSON.h"
#include "math/Axis.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"
#include "palette/RGBABuffer.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphUtil.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "util/VarUtil.h"
#include "voxelformat/private/mesh/GodotSceneFormat.h"
#include <SDL3/SDL_stdinc.h>

namespace voxelformat {

namespace priv {

enum class PxoLayerType { Pixel = 0, Group = 1, ThreeD = 2, Tilemap = 3, Audio = 4 };

struct PxoLayer {
	core::String name;
	int type = (int)PxoLayerType::Pixel;
	int parent = -1;
	bool visible = true;
	float opacity = 1.0f;
};

static color::RGBA blendOver(color::RGBA dst, color::RGBA src, float opacity) {
	if (src.a == 0 || opacity <= 0.0f) {
		return dst;
	}
	const float srcA = ((float)src.a / 255.0f) * opacity;
	const float dstA = (float)dst.a / 255.0f;
	const float outA = srcA + dstA * (1.0f - srcA);
	if (outA <= 0.0f) {
		return color::RGBA(0, 0, 0, 0);
	}
	const float inv = 1.0f / outA;
	const float srcFactor = srcA * inv;
	const float dstFactor = dstA * (1.0f - srcA) * inv;
	return color::RGBA((uint8_t)(src.r * srcFactor + dst.r * dstFactor),
					   (uint8_t)(src.g * srcFactor + dst.g * dstFactor),
					   (uint8_t)(src.b * srcFactor + dst.b * dstFactor), (uint8_t)(outA * 255.0f + 0.5f));
}

/**
 * Pixelorama serializes Godot Vector2i as either "Vector2i(x, y)" (via var_to_str)
 * or "(x, y)" (plain stringification of Vector2i).
 */
static bool parseVec2i(const char *str, int &x, int &y) {
	if (str == nullptr || str[0] == '\0') {
		return false;
	}
	const char *paren = SDL_strchr(str, '(');
	if (paren == nullptr) {
		return false;
	}
	return SDL_sscanf(paren, "(%i, %i)", &x, &y) == 2;
}

static bool isDrawableLayer(int type) {
	return type == (int)PxoLayerType::Pixel || type == (int)PxoLayerType::Tilemap;
}

static bool hasOpaquePixels(const image::ImagePtr &image) {
	if (!image || !image->isLoaded()) {
		return false;
	}
	for (int y = 0; y < image->height(); ++y) {
		for (int x = 0; x < image->width(); ++x) {
			if (image->colorAt(x, y).a != 0) {
				return true;
			}
		}
	}
	return false;
}

/**
 * Voxelorama stores per-pixel extrusion depth as cel metadata VoxelDepth:
 * Array of PackedFloat32Array columns -> JSON [[col0...], [col1...]] with
 * VoxelDepth[x][y]. Default depth is 1 (one voxel deep).
 * https://github.com/Orama-Interactive/VoxeloramaExtension
 */
static bool projectHasVoxelDepth(const json::Json &root) {
	if (!root.contains("frames") || !root.get("frames").isArray()) {
		return false;
	}
	for (const json::Json &frameJson : root.get("frames")) {
		const json::Json celsJson = frameJson.get("cels");
		if (!celsJson.isArray()) {
			continue;
		}
		for (const json::Json &celJson : celsJson) {
			const json::Json meta = celJson.get("metadata");
			if (meta.isObject() && meta.contains("VoxelDepth")) {
				return true;
			}
		}
	}
	return false;
}

static bool parseVoxelDepth(const json::Json &celJson, int width, int height, core::Buffer<float> &depths) {
	depths = core::Buffer<float>((size_t)width * (size_t)height, 0);
	for (size_t i = 0; i < depths.size(); ++i) {
		depths[i] = 1.0f;
	}
	const json::Json meta = celJson.get("metadata");
	if (!meta.isObject() || !meta.contains("VoxelDepth")) {
		return false;
	}
	const json::Json depthJson = meta.get("VoxelDepth");
	if (!depthJson.isArray() || depthJson.size() != width) {
		Log::warn("Ignoring VoxelDepth with unexpected size (got %i columns, expected %i)",
				  depthJson.isArray() ? depthJson.size() : -1, width);
		return false;
	}
	for (int x = 0; x < width; ++x) {
		const json::Json col = depthJson.get(x);
		if (!col.isArray() || col.size() != height) {
			Log::warn("Ignoring VoxelDepth column %i with unexpected size", x);
			return false;
		}
		for (int y = 0; y < height; ++y) {
			depths[(size_t)x * (size_t)height + (size_t)y] = col.get(y).floatVal();
		}
	}
	return true;
}

static int voxelDepthAt(const core::Buffer<float> &depths, int width, int height, int x, int y) {
	if (x < 0 || y < 0 || x >= width || y >= height || depths.empty()) {
		return 1;
	}
	const float d = depths[(size_t)x * (size_t)height + (size_t)y];
	const int depth = (int)(d + 0.5f);
	return depth < 1 ? 1 : depth;
}

static image::ImagePtr loadRawRGBA(const io::ArchivePtr &zip, const core::String &path, int width, int height) {
	core::ScopedPtr<io::SeekableReadStream> stream(zip->readStream(path));
	if (!stream) {
		return image::ImagePtr();
	}
	const int64_t expected = (int64_t)width * (int64_t)height * 4;
	if (width <= 0 || height <= 0 || stream->size() < expected) {
		Log::error("Failed to load RGBA image '%s' (%dx%d), stream size %i (expected %i)", path.c_str(), width, height,
				   (int)stream->size(), (int)expected);
		return image::ImagePtr();
	}
	image::ImagePtr image = image::createEmptyImage(path);
	if (!image->loadRGBA(*stream, width, height)) {
		Log::error("Failed to load RGBA image '%s' (%dx%d)", path.c_str(), width, height);
		return image::ImagePtr();
	}
	return image;
}

struct PxoCelImage {
	image::ImagePtr image;
	int offsetX = 0;
	int offsetY = 0;
	int zBase = 0;
	core::Buffer<float> depths; // empty => treat as depth 1
	bool hasDepth = false;
};

static bool loadCelImage(const io::ArchivePtr &zip, const json::Json &celJson, int canvasWidth, int canvasHeight,
						 int frameIndex, int layerIdx, PxoCelImage &out) {
	int imageWidth = canvasWidth;
	int imageHeight = canvasHeight;
	if (celJson.contains("image_size")) {
		int w = canvasWidth;
		int h = canvasHeight;
		const json::Json sizeJson = celJson.get("image_size");
		if (sizeJson.isString() && parseVec2i(sizeJson.cStr(), w, h) && w > 0 && h > 0) {
			imageWidth = w;
			imageHeight = h;
		}
	}
	out.offsetX = 0;
	out.offsetY = 0;
	if (celJson.contains("offset")) {
		const json::Json offsetJson = celJson.get("offset");
		if (offsetJson.isString()) {
			parseVec2i(offsetJson.cStr(), out.offsetX, out.offsetY);
		} else if (offsetJson.isArray() && offsetJson.size() >= 2) {
			out.offsetX = offsetJson.get(0).intVal();
			out.offsetY = offsetJson.get(1).intVal();
		}
	}
	const core::String path =
		core::String::format("image_data/frames/%i/layer_%i", frameIndex + 1, layerIdx + 1);
	if (!zip->exists(path)) {
		return false;
	}
	out.image = loadRawRGBA(zip, path, imageWidth, imageHeight);
	if (!out.image) {
		return false;
	}
	out.hasDepth = parseVoxelDepth(celJson, imageWidth, imageHeight, out.depths);
	return true;
}

static bool loadLayers(const json::Json &root, core::DynamicArray<PxoLayer> &layers) {
	if (!root.contains("layers") || !root.get("layers").isArray()) {
		Log::error("Missing layers array in data.json");
		return false;
	}
	const json::Json layersJson = root.get("layers");
	layers.reserve(layersJson.size());
	for (const json::Json &layerJson : layersJson) {
		PxoLayer layer;
		layer.name = layerJson.strVal("name", "Layer");
		layer.type = layerJson.intVal("type", (int)PxoLayerType::Pixel);
		layer.parent = layerJson.intVal("parent", -1);
		layer.visible = layerJson.boolVal("visible", true);
		layer.opacity = layerJson.floatVal("opacity", 1.0f);
		layers.push_back(layer);
	}
	return !layers.empty();
}

static image::ImagePtr loadFrameImage(const io::ArchivePtr &zip, const json::Json &root,
									  const core::DynamicArray<PxoLayer> &layers, int width, int height,
									  int frameIndex) {
	const core::String finalPath = core::String::format("image_data/final_images/%i", frameIndex + 1);
	if (zip->exists(finalPath)) {
		image::ImagePtr image = loadRawRGBA(zip, finalPath, width, height);
		if (image) {
			return image;
		}
	}

	core::Buffer<uint8_t> empty(width * height * 4);
	core_memset(empty.data(), 0, empty.size());
	image::ImagePtr composed = image::createEmptyImage(core::String::format("frame_%i", frameIndex + 1));
	if (!composed->loadRGBA(empty.data(), width, height)) {
		Log::error("Failed to allocate composed frame image");
		return image::ImagePtr();
	}

	if (!root.contains("frames") || !root.get("frames").isArray()) {
		Log::error("Missing frames array in data.json");
		return image::ImagePtr();
	}
	const json::Json framesJson = root.get("frames");
	if (frameIndex < 0 || frameIndex >= framesJson.size()) {
		Log::error("Frame index %i out of range", frameIndex);
		return image::ImagePtr();
	}
	const json::Json frameJson = framesJson.get(frameIndex);
	const json::Json celsJson = frameJson.get("cels");
	if (!celsJson.isArray()) {
		Log::error("Frame %i has no cels array", frameIndex);
		return image::ImagePtr();
	}

	const int layerCount = (int)layers.size();
	const int celCount = celsJson.size();
	for (int layerIdx = 0; layerIdx < layerCount && layerIdx < celCount; ++layerIdx) {
		const PxoLayer &layer = layers[layerIdx];
		if (!isDrawableLayer(layer.type) || !layer.visible || layer.opacity <= 0.0f) {
			continue;
		}
		const json::Json celJson = celsJson.get(layerIdx);
		const float celOpacity = celJson.floatVal("opacity", 1.0f);
		const float opacity = layer.opacity * celOpacity;
		if (opacity <= 0.0f) {
			continue;
		}

		// pxo_version 8+ stores shrunk cel images with image_size + offset.
		int imageWidth = width;
		int imageHeight = height;
		if (celJson.contains("image_size")) {
			int w = width;
			int h = height;
			const json::Json sizeJson = celJson.get("image_size");
			if (sizeJson.isString() && parseVec2i(sizeJson.cStr(), w, h) && w > 0 && h > 0) {
				imageWidth = w;
				imageHeight = h;
			}
		}

		int offsetX = 0;
		int offsetY = 0;
		if (celJson.contains("offset")) {
			const json::Json offsetJson = celJson.get("offset");
			if (offsetJson.isString()) {
				parseVec2i(offsetJson.cStr(), offsetX, offsetY);
			} else if (offsetJson.isArray() && offsetJson.size() >= 2) {
				offsetX = offsetJson.get(0).intVal();
				offsetY = offsetJson.get(1).intVal();
			}
		}

		const core::String path =
			core::String::format("image_data/frames/%i/layer_%i", frameIndex + 1, layerIdx + 1);
		if (!zip->exists(path)) {
			Log::debug("No pixel data for %s", path.c_str());
			continue;
		}
		image::ImagePtr layerImage = loadRawRGBA(zip, path, imageWidth, imageHeight);
		if (!layerImage) {
			continue;
		}

		for (int y = 0; y < layerImage->height(); ++y) {
			const int dstY = y + offsetY;
			if (dstY < 0 || dstY >= height) {
				continue;
			}
			for (int x = 0; x < layerImage->width(); ++x) {
				const int dstX = x + offsetX;
				if (dstX < 0 || dstX >= width) {
					continue;
				}
				const color::RGBA dst = composed->colorAt(dstX, dstY);
				const color::RGBA src = layerImage->colorAt(x, y);
				composed->setColor(blendOver(dst, src, opacity), dstX, dstY);
			}
		}
	}
	return composed;
}

} // namespace priv

bool PxoFormat::addFrame(scenegraph::SceneGraph &sceneGraph, const core::String &name, const palette::Palette &palette,
						 const image::ImagePtr &image, int frameIndex, math::Axis axis, int offset) const {
	if (!image || !image->isLoaded()) {
		Log::error("Invalid image for frame %i", frameIndex);
		return false;
	}
	const int w = image->width();
	const int h = image->height();
	voxel::Region region(0, 0, 0, w - 1, h - 1, 0);
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	palette::PaletteLookup palLookup(palette);
	voxel::RawVolume::Sampler sampler(volume);
	sampler.setPosition(0, h - 1, 0);
	for (int y = 0; y < h; ++y) {
		voxel::RawVolume::Sampler sampler2 = sampler;
		for (int x = 0; x < w; ++x) {
			const color::RGBA pixel = image->colorAt(x, y);
			if (pixel.a == 0) {
				sampler2.movePositiveX();
				continue;
			}
			const color::RGBA color = flattenRGB(pixel);
			const int index = palLookup.findClosestIndex(color);
			sampler2.setVoxel(voxel::createVoxel(palette, index));
			sampler2.movePositiveX();
		}
		sampler.moveNegativeY();
	}

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume);
	node.setName(core::String::format("%s_%i", name.c_str(), frameIndex));
	node.setPalette(palette);
	glm::ivec3 sliceOffset(0);
	sliceOffset[math::getIndexForAxis(axis)] = offset;
	sliceOffset *= frameIndex;
	node.volume()->translate(sliceOffset);
	return sceneGraph.emplace(core::move(node)) != InvalidNodeId;
}

bool PxoFormat::addVoxeloramaFrame(scenegraph::SceneGraph &sceneGraph, const core::String &name,
								  const palette::Palette &palette, const io::ArchivePtr &zip, const json::Json &root,
								  int canvasWidth, int canvasHeight, int frameIndex) const {
	core::DynamicArray<priv::PxoLayer> layers;
	if (!priv::loadLayers(root, layers)) {
		return false;
	}
	if (!root.contains("frames") || !root.get("frames").isArray()) {
		return false;
	}
	const json::Json framesJson = root.get("frames");
	if (frameIndex < 0 || frameIndex >= framesJson.size()) {
		return false;
	}
	const json::Json celsJson = framesJson.get(frameIndex).get("cels");
	if (!celsJson.isArray()) {
		return false;
	}

	core::DynamicArray<priv::PxoCelImage> cels;
	int maxDepth = 0;
	int layerZ = 0;
	const int layerCount = (int)layers.size();
	const int celCount = celsJson.size();
	for (int layerIdx = 0; layerIdx < layerCount && layerIdx < celCount; ++layerIdx) {
		const priv::PxoLayer &layer = layers[layerIdx];
		if (!priv::isDrawableLayer(layer.type) || !layer.visible || layer.opacity <= 0.0f) {
			continue;
		}
		priv::PxoCelImage cel;
		if (!priv::loadCelImage(zip, celsJson.get(layerIdx), canvasWidth, canvasHeight, frameIndex, layerIdx, cel)) {
			++layerZ;
			continue;
		}
		if (!priv::hasOpaquePixels(cel.image)) {
			++layerZ;
			continue;
		}
		int layerMax = 1;
		if (cel.hasDepth) {
			for (size_t i = 0; i < cel.depths.size(); ++i) {
				const int d = (int)(cel.depths[i] + 0.5f);
				if (d > layerMax) {
					layerMax = d;
				}
			}
		}
		if (layerZ + layerMax > maxDepth) {
			maxDepth = layerZ + layerMax;
		}
		cel.zBase = layerZ;
		cels.push_back(core::move(cel));
		++layerZ;
	}
	if (cels.empty() || maxDepth <= 0) {
		return false;
	}

	voxel::Region region(0, 0, 0, canvasWidth - 1, canvasHeight - 1, maxDepth - 1);
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	palette::PaletteLookup palLookup(palette);
	int filled = 0;
	for (size_t ci = 0; ci < cels.size(); ++ci) {
		const priv::PxoCelImage &cel = cels[ci];
		const int imgW = cel.image->width();
		const int imgH = cel.image->height();
		for (int y = 0; y < imgH; ++y) {
			const int canvasY = y + cel.offsetY;
			if (canvasY < 0 || canvasY >= canvasHeight) {
				continue;
			}
			for (int x = 0; x < imgW; ++x) {
				const int canvasX = x + cel.offsetX;
				if (canvasX < 0 || canvasX >= canvasWidth) {
					continue;
				}
				const color::RGBA pixel = cel.image->colorAt(x, y);
				if (pixel.a == 0) {
					continue;
				}
				const int depth = priv::voxelDepthAt(cel.depths, imgW, imgH, x, y);
				const color::RGBA color = flattenRGB(pixel);
				const int index = palLookup.findClosestIndex(color);
				const voxel::Voxel voxel = voxel::createVoxel(palette, index);
				const int voxelY = canvasHeight - 1 - canvasY;
				for (int z = 0; z < depth; ++z) {
					const int vz = cel.zBase + z;
					if (vz >= maxDepth) {
						break;
					}
					volume->setVoxel(canvasX, voxelY, vz, voxel);
					++filled;
				}
			}
		}
	}
	if (filled == 0) {
		delete volume;
		return false;
	}

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume);
	node.setName(core::String::format("%s_%i", name.c_str(), frameIndex));
	node.setPalette(palette);
	node.setProperty("voxelorama", "true");
	return sceneGraph.emplace(core::move(node)) != InvalidNodeId;
}

bool PxoFormat::loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive,
							   scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
							   const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	const io::ArchivePtr &zip = io::openZipArchive(stream);
	if (!zip) {
		Log::error("Failed to open pxo zip archive '%s'", filename.c_str());
		return false;
	}

	core::ScopedPtr<io::SeekableReadStream> jsonStream(zip->readStream("data.json"));
	if (!jsonStream) {
		Log::error("Missing data.json in '%s'", filename.c_str());
		return false;
	}
	core::String jsonString;
	if (!jsonStream->readString(jsonStream->remaining(), jsonString)) {
		Log::error("Failed to read data.json from '%s'", filename.c_str());
		return false;
	}
	const json::Json root = json::Json::parse(jsonString);
	if (!root.isValid() || !root.isObject()) {
		Log::error("Failed to parse data.json from '%s'", filename.c_str());
		return false;
	}

	const int width = root.intVal("size_x", 0);
	const int height = root.intVal("size_y", 0);
	if (width <= 0 || height <= 0) {
		Log::error("Invalid canvas size %dx%d in '%s'", width, height, filename.c_str());
		return false;
	}

	core::DynamicArray<priv::PxoLayer> layers;
	if (!priv::loadLayers(root, layers)) {
		return false;
	}
	if (!root.contains("frames") || !root.get("frames").isArray() || root.get("frames").size() <= 0) {
		Log::error("No frames in '%s'", filename.c_str());
		return false;
	}

	const int frameCount = root.get("frames").size();
	const int offset = core::getVar(cfg::VoxformatImageSliceOffset)->intVal();
	const math::Axis axis = math::toAxis(core::getVar(cfg::VoxformatImageSliceOffsetAxis)->strVal());
	const core::String baseName = core::string::extractFilename(filename);

	int addedModels = 0;

	// Pixelorama stores each 3D layer as a Godot SubViewport scene under scene/<index>.
	int threeDIndex = 0;
	for (size_t layerIdx = 0; layerIdx < layers.size(); ++layerIdx) {
		const priv::PxoLayer &layer = layers[layerIdx];
		if (layer.type != (int)priv::PxoLayerType::ThreeD) {
			continue;
		}
		const core::String scenePath = core::String::format("scene/%i", threeDIndex);
		++threeDIndex;
		if (!layer.visible) {
			Log::debug("Skipping invisible 3D layer '%s'", layer.name.c_str());
			continue;
		}
		core::ScopedPtr<io::SeekableReadStream> sceneStream(zip->readStream(scenePath));
		if (!sceneStream) {
			Log::warn("Missing 3D layer scene '%s' for layer '%s'", scenePath.c_str(), layer.name.c_str());
			continue;
		}
		core::Buffer<uint8_t> sceneData((size_t)sceneStream->size());
		if (sceneStream->read(sceneData.data(), (int)sceneData.size()) != (int)sceneData.size()) {
			Log::error("Failed to read 3D layer scene '%s'", scenePath.c_str());
			return false;
		}
		const io::MemoryArchivePtr sceneArchive = io::openMemoryArchive();
		const core::String sceneFilename = core::String::format("%s.tscn", layer.name.c_str());
		if (!sceneArchive->add(sceneFilename, sceneData.data(), sceneData.size())) {
			Log::error("Failed to stage 3D layer scene '%s'", scenePath.c_str());
			return false;
		}

		scenegraph::SceneGraph layerGraph;
		GodotSceneFormat godotFormat;
		// Pixelorama BoxMeshes are typically unit-sized with ~64px albedo textures.
		// Scale up when the user left the mesh scale at its default so painted
		// detail survives voxelization.
		core::ScopedPtr<util::ScopedVarChange> scaleOverride;
		if (core::getVar(cfg::VoxformatScale)->floatVal() == 1.0f &&
			core::getVar(cfg::VoxformatScaleX)->floatVal() == 1.0f &&
			core::getVar(cfg::VoxformatScaleY)->floatVal() == 1.0f &&
			core::getVar(cfg::VoxformatScaleZ)->floatVal() == 1.0f &&
			core::getVar(cfg::VoxformatVoxelSize)->intVal() == 0) {
			scaleOverride = new util::ScopedVarChange(cfg::VoxformatScale, "64");
		}
		if (!godotFormat.load(sceneFilename, sceneArchive, layerGraph, ctx)) {
			Log::warn("Failed to voxelize 3D layer '%s' from '%s'", layer.name.c_str(), scenePath.c_str());
			continue;
		}
		const int modelCount = (int)layerGraph.size(scenegraph::SceneGraphNodeType::AllModels);
		if (modelCount == 0) {
			Log::warn("3D layer '%s' produced no models", layer.name.c_str());
			continue;
		}

		scenegraph::SceneGraphNode group(scenegraph::SceneGraphNodeType::Group);
		group.setName(layer.name.empty() ? core::String::format("3D Layer %i", threeDIndex) : layer.name);
		const int groupId = sceneGraph.emplace(core::move(group));
		if (groupId == InvalidNodeId) {
			return false;
		}
		const int transferred = scenegraph::addSceneGraphNodes(sceneGraph, layerGraph, groupId);
		if (transferred <= 0) {
			Log::warn("Failed to add voxelized 3D layer '%s' to scene graph", layer.name.c_str());
			continue;
		}
		addedModels += modelCount;
	}

	const bool voxelorama = priv::projectHasVoxelDepth(root);
	if (voxelorama) {
		Log::debug("Pixelorama file '%s' has Voxelorama VoxelDepth metadata", filename.c_str());
	}

	for (int frameIdx = 0; frameIdx < frameCount; ++frameIdx) {
		ctx.report("frame", frameIdx, frameCount);
		if (voxelorama) {
			if (addVoxeloramaFrame(sceneGraph, baseName, palette, zip, root, width, height, frameIdx)) {
				++addedModels;
			} else {
				Log::debug("Skipping empty Voxelorama frame %i in '%s'", frameIdx, filename.c_str());
			}
			continue;
		}
		const image::ImagePtr image = priv::loadFrameImage(zip, root, layers, width, height, frameIdx);
		if (!image) {
			Log::error("Failed to load frame %i from '%s'", frameIdx, filename.c_str());
			return false;
		}
		// Shrink-to-content cels can be fully transparent; skip empty frames instead of
		// creating large empty volumes.
		if (!priv::hasOpaquePixels(image)) {
			Log::debug("Skipping empty frame %i in '%s'", frameIdx, filename.c_str());
			continue;
		}
		if (!addFrame(sceneGraph, baseName, palette, image, frameIdx, axis, offset)) {
			return false;
		}
		++addedModels;
	}
	ctx.report("frame", frameCount, frameCount);

	if (addedModels == 0 && zip->exists("preview.png")) {
		Log::warn("No voxelized content in '%s'; falling back to preview.png", filename.c_str());
		core::ScopedPtr<io::SeekableReadStream> previewStream(zip->readStream("preview.png"));
		if (previewStream) {
			const image::ImagePtr preview = image::loadImage("preview.png", *previewStream);
			if (priv::hasOpaquePixels(preview)) {
				if (!addFrame(sceneGraph, baseName, palette, preview, 0, axis, offset)) {
					return false;
				}
				++addedModels;
			}
		}
	}

	if (addedModels == 0) {
		Log::error("No voxel content in '%s'", filename.c_str());
		return false;
	}
	return true;
}

size_t PxoFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
							  const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return 0u;
	}
	const io::ArchivePtr &zip = io::openZipArchive(stream);
	if (!zip) {
		Log::error("Failed to open pxo zip archive '%s'", filename.c_str());
		return 0u;
	}

	core::ScopedPtr<io::SeekableReadStream> jsonStream(zip->readStream("data.json"));
	if (!jsonStream) {
		Log::error("Missing data.json in '%s'", filename.c_str());
		return 0u;
	}
	core::String jsonString;
	if (!jsonStream->readString(jsonStream->remaining(), jsonString)) {
		Log::error("Failed to read data.json from '%s'", filename.c_str());
		return 0u;
	}
	const json::Json root = json::Json::parse(jsonString);
	if (!root.isValid() || !root.isObject()) {
		Log::error("Failed to parse data.json from '%s'", filename.c_str());
		return 0u;
	}

	const int width = root.intVal("size_x", 0);
	const int height = root.intVal("size_y", 0);
	if (width <= 0 || height <= 0) {
		Log::error("Invalid canvas size %dx%d in '%s'", width, height, filename.c_str());
		return 0u;
	}

	core::DynamicArray<priv::PxoLayer> layers;
	if (!priv::loadLayers(root, layers)) {
		return 0u;
	}
	if (!root.contains("frames") || !root.get("frames").isArray() || root.get("frames").size() <= 0) {
		Log::error("No frames in '%s'", filename.c_str());
		return 0u;
	}

	palette::RGBABuffer colors;
	const int frameCount = root.get("frames").size();
	auto collectColors = [this, &colors](const image::ImagePtr &image) {
		if (!image || !image->isLoaded()) {
			return;
		}
		for (int y = 0; y < image->height(); ++y) {
			for (int x = 0; x < image->width(); ++x) {
				const color::RGBA rgba = flattenRGB(image->colorAt(x, y));
				if (rgba.a == 0) {
					continue;
				}
				colors.put(rgba, true);
			}
		}
	};
	for (int frameIdx = 0; frameIdx < frameCount; ++frameIdx) {
		ctx.report("palette", frameIdx, frameCount);
		collectColors(priv::loadFrameImage(zip, root, layers, width, height, frameIdx));
	}
	ctx.report("palette", frameCount, frameCount);
	if (colors.empty() && zip->exists("preview.png")) {
		core::ScopedPtr<io::SeekableReadStream> previewStream(zip->readStream("preview.png"));
		if (previewStream) {
			collectColors(image::loadImage("preview.png", *previewStream));
		}
	}
	return createPalette(colors, palette);
}

image::ImagePtr PxoFormat::loadScreenshot(const core::String &filename, const io::ArchivePtr &archive,
										  const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return image::ImagePtr();
	}
	const io::ArchivePtr &zip = io::openZipArchive(stream);
	if (!zip) {
		Log::error("Failed to open pxo zip archive '%s'", filename.c_str());
		return image::ImagePtr();
	}
	if (!zip->exists("preview.png")) {
		return image::ImagePtr();
	}
	core::ScopedPtr<io::SeekableReadStream> previewStream(zip->readStream("preview.png"));
	if (!previewStream) {
		return image::ImagePtr();
	}
	return image::loadImage("preview.png", *previewStream);
}

bool PxoFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
						   const io::ArchivePtr &archive, const SaveContext &ctx) {
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	if (node == nullptr || node->volume() == nullptr) {
		Log::error("No model node found in scene graph");
		return false;
	}

	const voxel::RawVolume *volume = node->volume();
	const palette::Palette &palette = node->palette();
	const voxel::Region &region = volume->region();
	const int width = region.getWidthInVoxels();
	const int height = region.getHeightInVoxels();
	const int depth = region.getDepthInVoxels();

	io::BufferedReadWriteStream zipFileStream(64000);
	{
		io::ZipArchive zipArchive;
		if (!zipArchive.init(&zipFileStream)) {
			Log::error("Failed to initialize ZIP archive for writing");
			return false;
		}

		json::Json root = json::Json::object();
		root.set("pxo_version", 4);
		root.set("pixelorama_version", "vengi");
		root.set("size_x", width);
		root.set("size_y", height);
		root.set("fps", 6);
		root.set("export_file_name", core::string::extractFilename(filename));

		json::Json layers = json::Json::array();
		json::Json layer = json::Json::object();
		layer.set("name", node->name().empty() ? "Layer 1" : node->name());
		layer.set("type", (int)priv::PxoLayerType::Pixel);
		layer.set("parent", -1);
		layer.set("visible", true);
		layer.set("opacity", 1.0);
		layer.set("blend_mode", 0);
		layer.set("locked", false);
		layer.set("clipping_mask", false);
		layer.set("new_cels_linked", false);
		layers.push(layer);
		root.set("layers", layers);

		json::Json frames = json::Json::array();
		for (int z = 0; z < depth; ++z) {
			json::Json frame = json::Json::object();
			frame.set("duration", 1.0);
			json::Json cels = json::Json::array();
			json::Json cel = json::Json::object();
			cel.set("opacity", 1.0);
			cel.set("z_index", 0);
			cels.push(cel);
			frame.set("cels", cels);
			frames.push(frame);
		}
		root.set("frames", frames);

		{
			const core::String jsonOut = root.dump(1);
			core::ScopedPtr<io::SeekableWriteStream> jsonStream(zipArchive.writeStream("data.json"));
			if (!jsonStream || !jsonStream->writeString(jsonOut, false)) {
				Log::error("Failed to write data.json");
				return false;
			}
		}
		{
			core::ScopedPtr<io::SeekableWriteStream> mimeStream(zipArchive.writeStream("mimetype"));
			if (!mimeStream || !mimeStream->writeString("application/x-pixelorama", false)) {
				Log::error("Failed to write mimetype");
				return false;
			}
		}

		core::Buffer<uint8_t> empty(width * height * 4);
		core_memset(empty.data(), 0, empty.size());
		image::ImagePtr slice = image::createEmptyImage("slice");
		if (!slice->loadRGBA(empty.data(), width, height)) {
			Log::error("Failed to allocate slice image");
			return false;
		}
		for (int z = 0; z < depth; ++z) {
			ctx.report("frame", z, depth);
			for (int y = 0; y < height; ++y) {
				for (int x = 0; x < width; ++x) {
					const voxel::Voxel &voxel = volume->voxel(region.getLowerX() + x, region.getLowerY() + y,
															 region.getLowerZ() + z);
					color::RGBA color(0, 0, 0, 0);
					if (!voxel::isAir(voxel.getMaterial())) {
						color = palette.color(voxel.getColor());
					}
					// images are top-down
					slice->setColor(color, x, height - 1 - y);
				}
			}
			const core::String path = core::String::format("image_data/frames/%i/layer_1", z + 1);
			core::ScopedPtr<io::SeekableWriteStream> layerStream(zipArchive.writeStream(path));
			if (!layerStream) {
				Log::error("Failed to create %s", path.c_str());
				return false;
			}
			const int bytes = width * height * 4;
			if (layerStream->write(slice->data(), bytes) != bytes) {
				Log::error("Failed to write %s", path.c_str());
				return false;
			}
		}
		ctx.report("frame", depth, depth);
		zipArchive.shutdown();
	}

	core::ScopedPtr<io::SeekableWriteStream> outStream(archive->writeStream(filename));
	if (!outStream) {
		Log::error("Failed to create output stream for %s", filename.c_str());
		return false;
	}
	zipFileStream.seek(0);
	outStream->writeStream(zipFileStream);
	return true;
}

} // namespace voxelformat
