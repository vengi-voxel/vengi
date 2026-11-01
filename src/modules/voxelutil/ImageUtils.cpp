/**
 * @file
 */

#include "color/ColorUtil.h"
#include "ImageUtils.h"
#include "app/Async.h"
#include "core/Assert.h"
#include "core/Common.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/SharedPtr.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "core/collection/Buffer.h"
#include "image/Image.h"
#include "math/Axis.h"
#include "palette/Material.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"
#include "voxel/Face.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelutil/VolumeVisitor.h"
#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/vec4.hpp>

namespace voxelutil {

int importHeightMaxHeight(const image::ImagePtr &image, bool alphaAsHeight) {
	const int w = image->width();
	const int h = image->height();
	int maxHeight = 0;
	int minHeight = 255;
	if (alphaAsHeight) {
		for (int x = 0; x < w; ++x) {
			for (int y = 0; y < h; ++y) {
				const uint8_t alphaVal = image->colorAt(x, y).a;
				maxHeight = core_max(maxHeight, alphaVal);
				minHeight = core_min(minHeight, alphaVal);
			}
		}
	} else {
		for (int x = 0; x < w; ++x) {
			for (int y = 0; y < h; ++y) {
				const uint8_t redVal = image->colorAt(x, y).r;
				maxHeight = core_max(maxHeight, redVal);
				minHeight = core_min(minHeight, redVal);
			}
		}
	}
	if (maxHeight == minHeight) {
		return 1;
	}
	return maxHeight;
}

int getHeightValueFromAlpha(uint8_t alpha, bool adoptHeight, int volumeHeight, int minHeight) {
	int heightValue = adoptHeight ? (int)glm::round((float)alpha * (float)volumeHeight / 255.0f) : (int)alpha;
	if (heightValue < minHeight) {
		heightValue = minHeight;
	}
	return heightValue;
}

void importColoredHeightmap(voxel::RawVolumeWrapper &volume, const palette::Palette &palette,
							const image::ImagePtr &image, const voxel::Voxel &underground, uint8_t minHeight,
							bool adoptHeight) {
	palette::PaletteLookup palLookup(palette);
	auto fn = [&palLookup, &volume, &image, adoptHeight, minHeight, underground](int start, int end) {
		const voxel::Region &region = volume.region();
		const int volumeHeight = region.getHeightInVoxels();
		const int volumeWidth = region.getWidthInVoxels();
		const int volumeDepth = region.getDepthInVoxels();
		const glm::ivec3 &mins = region.getLowerCorner();
		const int imageWidth = image->width();
		const int imageHeight = image->height();
		const float stepWidthY = (float)imageHeight / (float)volumeDepth;
		const float stepWidthX = (float)imageWidth / (float)volumeWidth;
		float imageY = start * stepWidthY;

		voxel::RawVolumeWrapper::Sampler sampler(volume);
		for (int z = start; z < end; ++z, imageY += stepWidthY) {
			float imageX = 0.0f;
			for (int x = 0; x < volumeWidth; ++x, imageX += stepWidthX) {
				const color::RGBA heightmapPixel = image->colorAt((int)imageX, (int)imageY);
				const uint8_t heightValue =
					getHeightValueFromAlpha(heightmapPixel.a, adoptHeight, volumeHeight, minHeight);
				const color::RGBA rgba(heightmapPixel.r, heightmapPixel.g, heightmapPixel.b);
				const uint8_t palidx = palLookup.findClosestIndex(rgba);
				const voxel::Voxel surfaceVoxel = voxel::createVoxel(palLookup.palette(), palidx);
				if (voxel::isAir(underground.getMaterial())) {
					const glm::ivec3 pos(x, heightValue - 1, z);
					const glm::ivec3 regionPos = mins + pos;
					sampler.setPosition(regionPos);
					sampler.setVoxel(surfaceVoxel);
				} else {
					const glm::ivec3 pos(x, 0, z);
					const glm::ivec3 regionPos = mins + pos;
					sampler.setPosition(regionPos);

					for (int y = 0; y < heightValue; ++y) {
						voxel::Voxel voxel = underground;
						if (y >= heightValue - 1) {
							voxel = surfaceVoxel;
						}
						sampler.setVoxel(voxel);
						sampler.movePositiveY();
					}
				}
			}
		}
	};
	app::for_parallel(0, volume.region().getDepthInVoxels(), fn);
}

void importHeightmap(voxel::RawVolumeWrapper &volume, const image::ImagePtr &image, const voxel::Voxel &underground,
					 const voxel::Voxel &surface, uint8_t minHeight, bool adoptHeight) {
	const int maxImageHeight = importHeightMaxHeight(image, true);
	app::for_parallel(0, volume.region().getDepthInVoxels(), [&image, adoptHeight, minHeight, &volume, underground, surface, maxImageHeight] (int start, int end) {
		const int imageWidth = image->width();
		const int imageHeight = image->height();
		const voxel::Region &region = volume.region();
		const int volumeHeight = region.getHeightInVoxels();
		const int volumeWidth = region.getWidthInVoxels();
		const int volumeDepth = region.getDepthInVoxels();
		const glm::ivec3 &mins = region.getLowerCorner();
		const float stepWidthY = (float)imageHeight / (float)volumeDepth;
		const float stepWidthX = (float)imageWidth / (float)volumeWidth;
		Log::debug("stepwidth: %f %f", stepWidthX, stepWidthY);
		const float scaleHeight = adoptHeight ? (float)volumeHeight / (float)maxImageHeight : 1.0f;
		float imageY = 0.0f;
		voxel::RawVolumeWrapper::Sampler sampler(volume);
		sampler.setPosition(mins.x, mins.y, mins.z + start);
		for (int z = start; z < end; ++z, imageY += stepWidthY) {
			float imageX = 0.0f;
			voxel::RawVolumeWrapper::Sampler sampler2 = sampler;
			for (int x = 0; x < volumeWidth; ++x, imageX += stepWidthX) {
				voxel::RawVolumeWrapper::Sampler sampler3 = sampler2;
				const color::RGBA heightmapPixel = image->colorAt((int)imageX, (int)imageY);
				uint8_t heightValue = (uint8_t)(glm::round((float)(heightmapPixel.r) * scaleHeight));
				if (heightValue < minHeight) {
					heightValue = minHeight;
				}

				if (voxel::isAir(underground.getMaterial())) {
					sampler3.movePositiveY(heightValue - 1);
					sampler.setVoxel(surface);
				} else {
					for (int y = 0; y < heightValue; ++y) {
						voxel::Voxel voxel = surface;
						if (y < heightValue - 1) {
							voxel = underground;
						}
						sampler3.setVoxel(voxel);
						sampler3.movePositiveY();
					}
				}
				sampler2.movePositiveX();
			}
			sampler.movePositiveZ();
		}
	});
}

voxel::RawVolume *importAsPlane(const image::ImagePtr &image, const palette::Palette &palette, uint8_t thickness) {
	return importAsPlane(image.get(), palette, thickness);
}

voxel::RawVolume *importAsPlane(const image::Image *image, const palette::Palette &palette, uint8_t thickness) {
	if (thickness <= 0) {
		Log::error("Thickness can't be 0");
		return nullptr;
	}
	if (!image || !image->isLoaded()) {
		Log::error("No color image given");
		return nullptr;
	}
	if (image->components() != 4) {
		Log::error("Expected to get an rgba image");
		return nullptr;
	}
	const int imageWidth = image->width();
	const int imageHeight = image->height();
	if (imageWidth * imageHeight * thickness > 1024 * 1024 * 256) {
		Log::warn("Did not import plane - max volume size of 1024x1024 (thickness 256) exceeded (%i:%i:%i)", imageWidth,
				  imageHeight, thickness);
		return nullptr;
	}
	Log::debug("Import image as plane: w(%i), h(%i), d(%i)", imageWidth, imageHeight, thickness);
	const voxel::Region region(0, 0, 0, imageWidth - 1, imageHeight - 1, thickness - 1);
	voxel::RawVolume *volume = new voxel::RawVolume(region);

	palette::PaletteLookup palLookup(palette);
	app::for_parallel(0, imageWidth, [&image, &palLookup, &palette, &volume, imageHeight, thickness] (int start, int end) {
		voxel::RawVolume::Sampler sampler(volume);
		sampler.setPosition(start, imageHeight - 1, 0);
		for (int x = start; x < end; ++x) {
			voxel::RawVolume::Sampler sampler2 = sampler;
			for (int y = 0; y < imageHeight; ++y) {
				const color::RGBA data = image->colorAt(x, y);
				if (data.a == 0) {
					continue;
				}
				const uint8_t index = palLookup.findClosestIndex(data);
				const voxel::Voxel voxel = voxel::createVoxel(palette, index);
				voxel::RawVolume::Sampler sampler3 = sampler2;
				for (int z = 0; z < thickness; ++z) {
					sampler3.setVoxel(voxel);
					sampler3.movePositiveZ();
				}
				sampler2.moveNegativeY();
			}
			sampler.movePositiveX();
		}
	});
	return volume;
}

core::String getDefaultDepthMapFile(const core::String &imageName, const core::String &postfix) {
	return core::string::addPostfixToFile(imageName, postfix);
}

voxel::RawVolume *importAsVolume(const image::ImagePtr &image, const palette::Palette &palette, uint8_t maxDepth,
								 bool bothSides) {
	if (maxDepth <= 0) {
		Log::error("Max height can't be 0");
		return nullptr;
	}
	if (!image || !image->isLoaded()) {
		Log::error("No color image given");
		return nullptr;
	}
	const core::String &dmFile = getDefaultDepthMapFile(image->name());
	const image::ImagePtr &depthMap = image::loadImage(dmFile);
	if (!depthMap || !depthMap->isLoaded()) {
		Log::error("Couldn't load depthmap %s", dmFile.c_str());
		return nullptr;
	}
	return importAsVolume(image, depthMap, palette, maxDepth, bothSides);
}

voxel::RawVolume *importAsVolume(const image::ImagePtr &image, const image::ImagePtr &depthmap,
								 const palette::Palette &palette, uint8_t maxDepth, bool bothSides) {
	core_assert(image);
	core_assert(depthmap);
	if (!image->isLoaded()) {
		Log::error("Image '%s' is not loaded", image->name().c_str());
		return nullptr;
	}
	if (!depthmap->isLoaded()) {
		Log::error("Depthmap '%s' is not loaded", image->name().c_str());
		return nullptr;
	}

	if (depthmap->width() != image->width() || depthmap->height() != image->height()) {
		Log::error("Image dimensions differ for color and depthmap");
		return nullptr;
	}
	const int imageWidth = image->width();
	const int imageHeight = image->height();
	int volumeDepth = bothSides ? maxDepth * 2 + 1 : maxDepth;
	if (volumeDepth % 2 == 0) {
		Log::warn("Make max volume depth uneven");
		volumeDepth++;
	}
	if (imageWidth * imageHeight * volumeDepth > 1024 * 1024 * 256) {
		Log::warn("Did not import plane - max volume size of 1024x1024 (depth 256) exceeded (%i:%i:%i)", imageWidth,
				  imageHeight, volumeDepth);
		return nullptr;
	}
	Log::debug("Import image as volume: w(%i), h(%i), d(%i)", imageWidth, imageHeight, volumeDepth);
	const voxel::Region region(0, 0, 0, imageWidth - 1, imageHeight - 1, volumeDepth - 1);
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	palette::PaletteLookup palLookup(palette);
	auto fn = [&palLookup, &palette, imageWidth, volume, image, depthmap, maxDepth, bothSides] (int start, int end) {
		voxel::RawVolume::Sampler sampler(volume);
		sampler.setPosition(0, volume->region().getUpperY(), 0);
		for (int y = start; y < end; ++y) {
			voxel::RawVolume::Sampler sampler2 = sampler;
			for (int x = 0; x < imageWidth; ++x) {
				const color::RGBA data = image->colorAt(x, y);
				if (data.a == 0 /* AlphaThreshold */) {
					sampler2.movePositiveX();
					continue;
				}
				const uint8_t index = palLookup.findClosestIndex(data);
				const voxel::Voxel voxel = voxel::createVoxel(palette, index);
				const color::RGBA heightdata = depthmap->colorAt(x, y);
				const float thickness = (float)heightdata.r;
				const float maxthickness = maxDepth;
				const float height = thickness * maxthickness / 255.0f;
				if (bothSides) {
					const int heighti = (int)glm::ceil(height / 2.0f);
					const int minZ = maxDepth - heighti;
					const int maxZ = maxDepth + heighti;
					voxel::RawVolume::Sampler sampler3 = sampler2;
					for (int z = minZ; z <= maxZ; ++z) {
						sampler3.setVoxel(voxel);
						sampler3.movePositiveZ();
					}
				} else {
					const int heighti = (int)glm::ceil(height);
					voxel::RawVolume::Sampler sampler3 = sampler2;
					for (int z = volume->region().getLowerZ(); z < volume->region().getLowerZ() + heighti; ++z) {
						sampler3.setVoxel(voxel);
						sampler3.movePositiveZ();
					}
				}
				sampler2.movePositiveX();
			}
			sampler.moveNegativeY();
		}
	};
	app::for_parallel(0, imageHeight, fn);
	return volume;
}

namespace {
// 2:1 dimetric cube stamp (game isometric, not 120-degree true isometric).
// Tw = IsoSpriteWidth, Th = Tw / 2. Floor steps are 2 px across and 1 px down:
//   x = x0 + (a - b) * (Tw / 2)
//   y = y0 - (a + b) * (Th / 2) - h * (Tw / 2)
// https://pikuma.com/blog/isometric-projection-in-games
// 2-step pixel lines follow atan(0.5) ~ 26.57 deg (Slynyrd Pixelblog 41).
// Axis angles: 126.87 deg between floor axes, 116.57 deg to vertical.
// IsoHalfWidth = Tw / 2, IsoSlopeStep = Th / 2, IsoVerticalStep = Tw / 2 (cube rise).
constexpr int IsoSpriteWidth = 8;
constexpr int IsoSpriteHeight = 7;
constexpr int IsoHalfWidth = IsoSpriteWidth / 2;
constexpr int IsoSlopeStep = IsoHalfWidth / 2;
constexpr int IsoVerticalStep = IsoHalfWidth;

enum class IsoShade { Transparent, Top, Left, Right, Max };

// 8x7 wide-corner cube stamp (Slynyrd A1). Sharp D1 corners are not used.
// Pixel artwork for the top diamond and two side faces. Spaces are transparent.
// C = color, D = darker, L = lighter
const char *IsoSprite[IsoSpriteHeight] = {"  CCCC  ", "CCCCCCCC", "DDCCCCLL", "DDDDLLLL",
										  "DDDDLLLL", "DDDDLLLL", "  DDLL  "};

// View-space Lambert: visible cube normals are +Y (top), -X (left), -Z (right).
// Light sits above-left of the camera so the left wall is brighter than the right.
// shade = ambient + (1 - ambient) * max(0, N.L)
void isoShadeFactors(float lighting[(int)IsoShade::Max]) {
	const float lx = -0.6f;
	const float ly = 1.0f;
	const float lz = -0.2f;
	const float invLen = 1.0f / glm::sqrt(lx * lx + ly * ly + lz * lz);
	auto lambert = [&](float nx, float ny, float nz) {
		const float ndotl = (nx * lx + ny * ly + nz * lz) * invLen;
		return 0.40f + 0.60f * (ndotl > 0.0f ? ndotl : 0.0f);
	};
	lighting[(int)IsoShade::Transparent] = 0.0f;
	lighting[(int)IsoShade::Top] = lambert(0.0f, 1.0f, 0.0f);
	lighting[(int)IsoShade::Left] = lambert(-1.0f, 0.0f, 0.0f);
	lighting[(int)IsoShade::Right] = lambert(0.0f, 0.0f, -1.0f);
}

color::RGBA isoBlend(color::RGBA src, color::RGBA dst) {
	if (src.a >= 255) {
		return src;
	}
	if (src.a == 0) {
		return dst;
	}
	const int a = (int)src.a;
	const int ia = 255 - a;
	return color::RGBA((uint8_t)((src.r * a + dst.r * ia) / 255), (uint8_t)((src.g * a + dst.g * ia) / 255),
					   (uint8_t)((src.b * a + dst.b * ia) / 255),
					   (uint8_t)core_min(255, src.a + dst.a * ia / 255));
}

// Face lighting is applied in linear RGB. Materials mix toward vengi palette
// values: roughness flattens, metal increases contrast, emit mixes to unshaded.
color::RGBA isoShade(color::RGBA c, float shade, const palette::Material &mat, const IsoRenderConfig &config) {
	if (!config.shading) {
		shade = 1.0f;
	}
	if (config.materials) {
		const float roughness = glm::clamp(mat.roughness, 0.0f, 1.0f);
		const float metal = glm::clamp(mat.metal, 0.0f, 1.0f);
		const float emit = glm::clamp(mat.emit, 0.0f, 1.0f);
		shade = glm::mix(shade, 1.0f, roughness * 0.6f);
		shade = glm::mix(shade, shade * shade, metal);
		shade = glm::mix(shade, 1.0f, emit);
	}
	color::RGBA result = c;
	if (!config.transparency) {
		result.a = 255;
	}
	for (int i = 0; i < 3; ++i) {
		const float linear = color::srgbToLinear(c[i] / 255.0f);
		const float value = glm::clamp(linear * shade, 0.0f, 1.0f);
		const float srgb = value <= 0.0031308f ? value * 12.92f : 1.055f * glm::pow(value, 1.0f / 2.4f) - 0.055f;
		((uint8_t *)&result)[i] = (uint8_t)(srgb * 255.0f);
	}
	return result;
}

// View-space grid: a and b run along the floor, h is up. (0, 0, 0) is the
// near/bottom corner, so screen y decreases as a, b, or h grows.
// Empty: air or outside. Clear: palette alpha or a transparent voxel. Solid: opaque.
// Same rule as the cubic mesher: opaque faces show against empty and clear, clear faces
// show against empty and solid, and two clear voxels hide the face between them.
enum class IsoCover : uint8_t { Empty, Clear, Solid };

struct IsoView {
	const voxel::RawVolume *volume;
	const palette::Palette *palette;
	voxel::FaceNames face;
	bool transparency;
	int width;
	int depth;

	IsoView(const voxel::RawVolume *v, voxel::FaceNames frontFace, const palette::Palette &pal, bool useTransparency)
		: volume(v), palette(&pal), face(frontFace), transparency(useTransparency) {
		const voxel::Region &r = volume->region();
		const bool rotated = voxel::isX(face);
		width = rotated ? r.getDepthInVoxels() : r.getWidthInVoxels();
		depth = rotated ? r.getWidthInVoxels() : r.getDepthInVoxels();
	}

	IsoCover coverAt(int a, int b, int h) const {
		const glm::ivec3 p = position(a, b, h);
		if (!volume->region().containsPoint(p)) {
			return IsoCover::Empty;
		}
		const voxel::Voxel &voxel = volume->voxel(p);
		if (voxel::isAir(voxel.getMaterial())) {
			return IsoCover::Empty;
		}
		if (transparency &&
			(voxel::isTransparent(voxel.getMaterial()) || palette->color(voxel.getColor()).a < 255)) {
			return IsoCover::Clear;
		}
		return IsoCover::Solid;
	}

	static bool showFace(IsoCover current, IsoCover neighbor) {
		if (current == IsoCover::Empty) {
			return false;
		}
		if (current == IsoCover::Solid) {
			return neighbor != IsoCover::Solid;
		}
		return neighbor != IsoCover::Clear;
	}

	// Local bits match the stamp: PositiveY top (+h), NegativeX left (-a), NegativeZ right (-b).
	voxel::FaceBits visibleFaces(int a, int b, int h) const {
		const IsoCover current = coverAt(a, b, h);
		voxel::FaceBits result = voxel::FaceBits::None;
		if (showFace(current, coverAt(a + 1, b, h))) {
			result |= voxel::FaceBits::PositiveX;
		}
		if (showFace(current, coverAt(a - 1, b, h))) {
			result |= voxel::FaceBits::NegativeX;
		}
		if (showFace(current, coverAt(a, b, h + 1))) {
			result |= voxel::FaceBits::PositiveY;
		}
		if (showFace(current, coverAt(a, b, h - 1))) {
			result |= voxel::FaceBits::NegativeY;
		}
		if (showFace(current, coverAt(a, b + 1, h))) {
			result |= voxel::FaceBits::PositiveZ;
		}
		if (showFace(current, coverAt(a, b - 1, h))) {
			result |= voxel::FaceBits::NegativeZ;
		}
		return result;
	}

	glm::ivec3 position(int a, int b, int h) const {
		const voxel::Region &r = volume->region();
		const int y = face == voxel::FaceNames::Down ? r.getUpperY() - h : r.getLowerY() + h;
		// a is screen-right and follows the viewport camera: Front -X, Back +X, Left +Z, Right -Z.
		switch (face) {
		case voxel::FaceNames::Right:
			return {r.getUpperX() - b, y, r.getUpperZ() - a};
		case voxel::FaceNames::Back:
			return {r.getLowerX() + a, y, r.getUpperZ() - b};
		case voxel::FaceNames::Left:
			return {r.getLowerX() + b, y, r.getLowerZ() + a};
		default:
			return {r.getUpperX() - a, y, r.getLowerZ() + b};
		}
	}

	bool occupied(int a, int b, int h) const {
		const glm::ivec3 p = position(a, b, h);
		return volume->region().containsPoint(p) && !voxel::isAir(volume->voxel(p).getMaterial());
	}
};

void applyIsoOutline(image::Image &image, const core::Buffer<uint8_t> &covered, const IsoRenderConfig &config) {
	const int width = image.width();
	const int height = image.height();
	const int step[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			if (covered[y * width + x] == 0) {
				continue;
			}
			bool ink = false;
			for (int n = 0; n < 4; ++n) {
				const int nx = x + step[n][0];
				const int ny = y + step[n][1];
				if (nx < 0 || ny < 0 || nx >= width || ny >= height || covered[ny * width + nx] == 0) {
					ink = true;
					break;
				}
			}
			if (!ink) {
				continue;
			}
			color::RGBA inkColor = config.outlineColor;
			if (!config.transparency) {
				inkColor.a = 255;
			}
			image.setColor(inkColor, x, y);
		}
	}
}
} // namespace

static void applySharedImageCvars(color::RGBA &background, int &imgW, int &imgH, bool &upScale) {
	if (const core::VarPtr &v = core::Var::findVar(cfg::ImageBackground)) {
		float c[4];
		v->vec4Val(c);
		background = color::getRGBA(glm::vec4(c[0], c[1], c[2], c[3]));
	}
	if (const core::VarPtr &v = core::Var::findVar(cfg::ImageWidth)) {
		imgW = v->intVal();
	}
	if (const core::VarPtr &v = core::Var::findVar(cfg::ImageHeight)) {
		imgH = v->intVal();
	}
	if (const core::VarPtr &v = core::Var::findVar(cfg::ImageUpScale)) {
		upScale = v->boolVal();
	}
}

ImageRenderConfig ImageRenderConfig::fromCvars() {
	ImageRenderConfig config;
	applySharedImageCvars(config.background, config.imgW, config.imgH, config.upScale);
	if (const core::VarPtr &v = core::Var::findVar(cfg::ImageDepthFactor2D)) {
		config.depthFactor = v->floatVal();
	}
	return config;
}

IsoRenderConfig IsoRenderConfig::fromCvars() {
	IsoRenderConfig config;
	applySharedImageCvars(config.background, config.imgW, config.imgH, config.upScale);
	if (const core::VarPtr &v = core::Var::findVar(cfg::ImageIsoShading)) {
		config.shading = v->boolVal();
	}
	if (const core::VarPtr &v = core::Var::findVar(cfg::ImageIsoMaterials)) {
		config.materials = v->boolVal();
	}
	if (const core::VarPtr &v = core::Var::findVar(cfg::ImageIsoTransparency)) {
		config.transparency = v->boolVal();
	}
	if (const core::VarPtr &v = core::Var::findVar(cfg::ImageIsoOutline)) {
		config.outline = v->boolVal();
	}
	if (const core::VarPtr &v = core::Var::findVar(cfg::ImageIsoOutlineColor)) {
		float c[4];
		v->vec4Val(c);
		config.outlineColor = color::getRGBA(glm::vec4(c[0], c[1], c[2], c[3]));
	}
	return config;
}

image::ImagePtr renderIsometricImage(const voxel::RawVolume *volume, const palette::Palette &palette,
									 const IsoRenderConfig &config) {
	if (volume == nullptr || !volume->region().isValid() || config.frontFace == voxel::FaceNames::Max) {
		return {};
	}
	const voxel::Region &r = volume->region();
	const IsoView view(volume, config.frontFace, palette, config.transparency);
	const int height = r.getHeightInVoxels();
	image::ImagePtr image = image::createEmptyImage("isometric");
	// Diamond span (width + depth) * (Tw / 2), plus one cube rise per layer.
	image->resize(IsoHalfWidth * (view.width + view.depth),
				  IsoSlopeStep * (view.width + view.depth - 2) + IsoVerticalStep * (height - 1) + IsoSpriteHeight);
	// TODO: PERF: add image->fill(background) here to do a memset only
	for (int y = 0; y < image->height(); ++y) {
		for (int x = 0; x < image->width(); ++x) {
			image->setColor(config.background, x, y);
		}
	}

	color::RGBA shades[palette::PaletteMaxColors][(int)IsoShade::Max];
	float lighting[(int)IsoShade::Max];
	isoShadeFactors(lighting);
	for (int i = 0; i < palette::PaletteMaxColors; ++i) {
		const palette::Material &mat = palette.material(i);
		for (int j = 0; j < (int)IsoShade::Max; ++j) {
			shades[i][j] = isoShade(palette.color(i), lighting[j], mat, config);
		}
	}

	core::Buffer<uint8_t> covered;
	if (config.outline) {
		covered = core::Buffer<uint8_t>((size_t)image->width() * (size_t)image->height(), 0);
	}
	const int imageWidth = image->width();

	// Painter's algorithm: far to near in all three view axes. Voxels that
	// overlap on screen lie along (a+1,b+1,h-1), so this order resolves
	// occlusion without sorting.
	for (int a = view.width - 1; a >= 0; --a) {
		for (int b = view.depth - 1; b >= 0; --b) {
			for (int h = 0; h < height; ++h) {
				if (!view.occupied(a, b, h)) {
					continue;
				}
				const voxel::FaceBits visible = view.visibleFaces(a, b, h);
				const bool top = (visible & voxel::FaceBits::PositiveY) != voxel::FaceBits::None;
				const bool left = (visible & voxel::FaceBits::NegativeX) != voxel::FaceBits::None;
				const bool right = (visible & voxel::FaceBits::NegativeZ) != voxel::FaceBits::None;
				if (!top && !left && !right) {
					continue;
				}
				const voxel::Voxel &v = volume->voxel(view.position(a, b, h));
				const color::RGBA *colors = shades[v.getColor()];
				// x = (Tw / 2) * ((depth - 1) + a - b)
				// y = (Th / 2) * ((width + depth - 2) - a - b) + (Tw / 2) * (height - 1 - h)
				const int x = IsoHalfWidth * (view.depth - 1 + a - b);
				const int y = IsoSlopeStep * (view.width + view.depth - 2 - a - b) + IsoVerticalStep * (height - 1 - h);
				for (int j = 0; j < IsoSpriteHeight; ++j) {
					for (int i = 0; i < IsoSpriteWidth; ++i) {
						IsoShade shade = IsoShade::Transparent;
						switch (IsoSprite[j][i]) {
						case 'C':
							if (top) {
								shade = IsoShade::Top;
							}
							break;
						case 'D':
							if (left) {
								shade = IsoShade::Left;
							}
							break;
						case 'L':
							if (right) {
								shade = IsoShade::Right;
							}
							break;
						default:
							break;
						}
						if (shade == IsoShade::Transparent) {
							continue;
						}
						const int px = x + i;
						const int py = y + j;
						const color::RGBA src = colors[(int)shade];
						if (config.transparency && src.a < 255) {
							image->setColor(isoBlend(src, image->colorAt(px, py)), px, py);
						} else {
							image->setColor(src, px, py);
						}
						if (config.outline) {
							covered[py * imageWidth + px] = 1;
						}
					}
				}
			}
		}
	}

	if (config.outline) {
		applyIsoOutline(*image.get(), covered, config);
	}

	const int width = image->width();
	const int nativeHeight = image->height();
	int imgW = config.imgW;
	int imgH = config.imgH;
	if (imgW > 0 || imgH > 0) {
		if (imgW <= 0) {
			imgW = core_max(1, (int)glm::round((float)width * imgH / nativeHeight));
		}
		if (imgH <= 0) {
			imgH = core_max(1, (int)glm::round((float)nativeHeight * imgW / width));
		}
		if (config.upScale || (imgW <= width && imgH <= nativeHeight)) {
			// Nearest sampling preserves the discrete shades and transparent silhouette.
			image::ImagePtr scaled = image::createEmptyImage("isometric");
			scaled->resize(imgW, imgH);
			for (int y = 0; y < imgH; ++y) {
				for (int x = 0; x < imgW; ++x) {
					scaled->setColor(
						image->colorAt((int)((int64_t)x * width / imgW), (int)((int64_t)y * nativeHeight / imgH)), x,
						y);
				}
			}
			image = scaled;
		}
	}

	// finally mark this as loaded to indicate that the image data is valid
	image->markLoaded();
	return image;
}

image::ImagePtr renderFaceToImage(const voxel::RawVolume *volume, const palette::Palette &palette,
								  const voxel::Region &region, voxel::FaceNames face) {
	if (face == voxel::FaceNames::Max) {
		Log::error("Invalid face for renderFaceToImage");
		return image::ImagePtr();
	}

	const glm::ivec3 &dim = region.getDimensionsInVoxels();
	int width = 1;
	int height = 1;
	if (voxel::isY(face)) {
		width = dim.x;
		height = dim.z;
	} else if (voxel::isX(face)) {
		width = dim.z;
		height = dim.y;
	} else if (voxel::isZ(face)) {
		width = dim.x;
		height = dim.y;
	}

	image::ImagePtr image = core::make_shared<image::Image>("renderFaceToImage");
	image->resize(width, height);
	for (int x = 0; x < width; ++x) {
		for (int y = 0; y < height; ++y) {
			image->setColor(color::RGBA(0, 0, 0, 0), x, y);
		}
	}

	const glm::ivec3 &mins = region.getLowerCorner();
	voxelutil::visitFace(*volume, region, face, [face, &palette, &image, &mins, width, height](int x, int y, int z, const voxel::Voxel &voxel) {
		const color::RGBA rgba = palette.color(voxel.getColor());
		int px = 0;
		int py = 0;
		if (voxel::isY(face)) {
			px = width - 1 - (x - mins.x);
			py = voxel::isPositiveFace(face) ? height - 1 - (z - mins.z) : (z - mins.z);
		} else if (voxel::isX(face)) {
			px = voxel::isPositiveFace(face) ? width - 1 - (z - mins.z) : (z - mins.z);
			py = height - 1 - (y - mins.y);
		} else if (voxel::isZ(face)) {
			px = voxel::isPositiveFace(face) ? (x - mins.x) : width - 1 - (x - mins.x);
			py = height - 1 - (y - mins.y);
		}
		image->setColor(rgba, px, py);
	}, true);

	image->markLoaded();
	Log::debug("Created face texture image with dimensions %ix%i", width, height);
	return image;
}

image::ImagePtr renderToImage(const voxel::RawVolume *volume, const palette::Palette &palette,
							  const ImageRenderConfig &config) {
	const voxel::FaceNames frontFace = config.frontFace;
	const color::RGBA background = config.background;
	int imgW = config.imgW;
	int imgH = config.imgH;
	const bool upScale = config.upScale;
	const float depthFactor = config.depthFactor;
	image::ImagePtr image = core::make_shared<image::Image>("renderToImage");
	const voxel::Region &region = volume->region();
	const glm::ivec3 &dim = region.getDimensionsInVoxels();
	int width = 1;
	int height = 1;
	if (voxel::isY(frontFace)) {
		width = dim.x;
		height = dim.z;
	} else if (voxel::isX(frontFace)) {
		width = dim.z;
		height = dim.y;
	} else if (voxel::isZ(frontFace)) {
		width = dim.x;
		height = dim.y;
	}

	// now that we have the size - fill it with the background color
	image->resize(width, height);
	for (int x = 0; x < width; ++x) {
		for (int y = 0; y < height; ++y) {
			image->setColor(background, x, y);
		}
	}

	// now render the voxels to the image
	voxelutil::visitFace(*volume, frontFace, [frontFace, &palette, &image, &region, width, height, depthFactor] (int x, int y, int z, const voxel::Voxel& voxel) {
		color::RGBA rgba = palette.color(voxel.getColor());
		int px = 0;
		int py = 0;
		float depth = 0.0f;
		if (voxel::isY(frontFace)) {
			px = width - 1 - (x - region.getLowerX());
			py = voxel::isPositiveFace(frontFace) ? height - 1 - (z - region.getLowerZ()) : (z - region.getLowerZ());
			if (voxel::isPositiveFace(frontFace)) {
				depth = (float)((region.getUpperY() - y) * depthFactor) / (float)region.getHeightInVoxels();
			} else {
				depth = (float)((y - region.getLowerY()) * depthFactor) / (float)region.getHeightInVoxels();
			}
		} else if (voxel::isX(frontFace)) {
			px = voxel::isPositiveFace(frontFace) ? width - 1 - (z - region.getLowerZ()) : (z - region.getLowerZ());
			py = height - 1 - (y - region.getLowerY());
			if (voxel::isPositiveFace(frontFace)) {
				depth = (float)((region.getUpperX() - x) * depthFactor) / (float)region.getWidthInVoxels();
			} else {
				depth = (float)((x - region.getLowerX()) * depthFactor) / (float)region.getWidthInVoxels();
			}

		} else if (voxel::isZ(frontFace)) {
			px = voxel::isPositiveFace(frontFace) ? (x - region.getLowerX()) : width - 1 - (x - region.getLowerX());
			py = height - 1 - (y - region.getLowerY());
			if (voxel::isPositiveFace(frontFace)) {
				depth = (float)((region.getUpperZ() - z) * depthFactor) / (float)region.getDepthInVoxels();
			} else {
				depth = (float)((z - region.getLowerZ()) * depthFactor) / (float)region.getDepthInVoxels();
			}
		} else {
			return;
		}

		if (depthFactor > 0.0f) {
			rgba = color::darker(rgba, depth);
		}
		image->setColor(rgba, px, py);
	}, VisitorOrder::Max, true);

	// check if we need to rescale the image
	if ((imgW > 0 && imgW != width) || (imgH > 0 && imgH != height)) {
		if (imgW <= 0) {
			const float factor = (float)imgH / (float)height;
			imgW = (int)glm::round((float)width * factor);
		}
		if (imgH <= 0) {
			const float factor = (float)imgW / (float)width;
			imgH = (int)glm::round((float)height * factor);
		}
		const bool wouldUpscale = imgW > width || imgH > height;
		if (upScale || !wouldUpscale) {
			image->resize(imgW, imgH);
		}
	}

	// finally mark this as loaded to indicate that the image data is valid
	image->markLoaded();

	return image;
}

} // namespace voxelutil
