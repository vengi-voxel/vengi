/**
 * @file
 */

#include "PaintBrush.h"
#include "color/ColorUtil.h"
#include "color/RGBA.h"
#include "core/collection/DynamicArray.h"
#include "palette/Palette.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxelutil/VoxelUtil.h"
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace voxedit {

voxel::Voxel PaintBrush::VoxelColor::evaluate(const voxel::Voxel &old) {
	if (_paintMode == PaintMode::Replace) {
		return _voxel;
	}
	if (_paintMode == PaintMode::Random) {
		int n = _palette.colorCount();
		int idx = rand() % n;
		return voxel::createVoxel(_palette, idx, old.getFlags());
	}

	bool brighten = _paintMode == PaintMode::Brighten;
	if (_paintMode == PaintMode::Variation) {
		if (rand() % _variationChance != 0) {
			return old;
		}
		brighten = rand() % 2 == 0;
	}

	const color::RGBA voxelColor = _palette.color(old.getColor());
	color::RGBA newColor;
	if (brighten) {
		newColor = color::brighter(voxelColor, _strength);
	} else {
		newColor = color::darker(voxelColor, _strength);
	}
	const int index = _palette.getClosestMatch(newColor, old.getColor());
	if (index == palette::PaletteColorNotFound) {
		uint8_t newColorIndex = 0;
		if (!_palette.tryAdd(newColor, false, &newColorIndex, false, old.getColor())) {
			return old;
		}
		_palette.markDirty();
		_palette.markSave();
		// TODO: MEMENTO: no memento state handling for the palette here
		return voxel::createVoxel(_palette, newColorIndex, old.getFlags());
	}
	return voxel::createVoxel(_palette, index, old.getFlags());
}

static voxel::Voxel mix(ModifierVolumeWrapper &wrapper, const voxel::Voxel &from, const voxel::Voxel &to,
						float factor) {
	const palette::Palette &palette = wrapper.node().palette();
	const glm::vec4 colorA = palette.color4(from.getColor());
	const glm::vec4 colorB = palette.color4(to.getColor());
	const glm::vec4 newColor = glm::mix(colorA, colorB, factor);
	const int index = palette.getClosestMatch(color::getRGBA(newColor), from.getColor());
	if (index == palette::PaletteColorNotFound) {
		return from;
	}
	return voxel::createVoxel(palette, index);
}

static voxel::Voxel mixBlend(palette::Palette &palette, const voxel::Voxel &from, const voxel::Voxel &to,
							 float factor) {
	if (factor <= 0.0f) {
		return from;
	}
	if (factor >= 1.0f) {
		return voxel::createVoxel(palette, to.getColor(), from.getNormal(), from.getFlags());
	}

	const color::RGBA colorA = palette.color(from.getColor());
	const color::RGBA colorB = palette.color(to.getColor());
	const color::RGBA newColor = color::RGBA::mix(colorA, colorB, factor);
	uint8_t newColorIndex = 0;
	if (palette.tryAdd(newColor, false, &newColorIndex, false, from.getColor())) {
		palette.markSave();
		// TODO: MEMENTO: no memento state handling for the palette here
		return voxel::createVoxel(palette, newColorIndex, from.getNormal(), from.getFlags());
	}
	const int index = palette.getClosestMatch(newColor, from.getColor());
	if (index == palette::PaletteColorNotFound) {
		return from;
	}
	return voxel::createVoxel(palette, index, from.getNormal(), from.getFlags());
}

static float blendFactor(const glm::ivec3 &pos, const glm::vec3 &center, float falloffRadius, float opacity) {
	if (falloffRadius <= 0.0f) {
		return opacity;
	}
	const float distance = glm::distance(glm::vec3(pos), center);
	const float t = glm::clamp(1.0f - distance / falloffRadius, 0.0f, 1.0f);
	return glm::smoothstep(0.0f, 1.0f, t) * opacity;
}

static voxel::Voxel gaussianBlurVoxel(ModifierVolumeWrapper &wrapper, palette::Palette &palette, int x, int y, int z,
									  const voxel::Voxel &old, float amount) {
	if (amount <= 0.0f) {
		return old;
	}

	// 3x3x3 Gaussian-ish kernel (separable weights approximated on Chebyshev neighborhood)
	static const float kWeights[3] = {0.25f, 0.5f, 0.25f};
	glm::vec3 sum(0.0f);
	float weightSum = 0.0f;
	for (int dz = -1; dz <= 1; ++dz) {
		for (int dy = -1; dy <= 1; ++dy) {
			for (int dx = -1; dx <= 1; ++dx) {
				const voxel::Voxel neighbor = wrapper.voxel(x + dx, y + dy, z + dz);
				if (voxel::isAir(neighbor.getMaterial())) {
					continue;
				}
				const float w = kWeights[dx + 1] * kWeights[dy + 1] * kWeights[dz + 1];
				const color::RGBA c = palette.color(neighbor.getColor());
				sum += glm::vec3((float)c.r, (float)c.g, (float)c.b) * w;
				weightSum += w;
			}
		}
	}
	if (weightSum <= 0.0f) {
		return old;
	}
	sum /= weightSum;
	const color::RGBA blurred((uint8_t)glm::clamp(sum.r, 0.0f, 255.0f), (uint8_t)glm::clamp(sum.g, 0.0f, 255.0f),
							  (uint8_t)glm::clamp(sum.b, 0.0f, 255.0f), 255);
	const color::RGBA original = palette.color(old.getColor());
	const color::RGBA mixed = color::RGBA::mix(original, blurred, amount);
	uint8_t newColorIndex = 0;
	if (palette.tryAdd(mixed, false, &newColorIndex, false, old.getColor())) {
		palette.markSave();
		// TODO: MEMENTO: no memento state handling for the palette here
		return voxel::createVoxel(palette, newColorIndex, old.getNormal(), old.getFlags());
	}
	const int index = palette.getClosestMatch(mixed, old.getColor());
	if (index == palette::PaletteColorNotFound) {
		return old;
	}
	return voxel::createVoxel(palette, index, old.getNormal(), old.getFlags());
}

bool PaintBrush::beginBrush(const BrushContext &ctx) {
	_blendStroke.clear();
	return Super::beginBrush(ctx);
}

void PaintBrush::endBrush(BrushContext &ctx) {
	_blendStroke.clear();
	Super::endBrush(ctx);
}

void PaintBrush::generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
						  const voxel::Region &region) {
	if (_paintMode == PaintMode::Blend || _paintMode == PaintMode::Blur) {
		palette::Palette &palette = wrapper.node().palette();
		// Only track stroke weights on the real volume; preview must not fill the map.
		const bool trackStroke = strokeActive() && !ctx.preview;
		glm::vec3 center;
		float falloffRadius;
		if (anyStrokeMode()) {
			center = glm::vec3(ctx.cursorPosition);
			falloffRadius = (float)radius();
		} else {
			center = glm::vec3(region.getCenter());
			const glm::vec3 halfExtents = glm::vec3(region.getDimensionsInVoxels()) * 0.5f;
			falloffRadius = glm::length(halfExtents);
		}

		core::DynamicArray<glm::ivec3> positions;
		core::DynamicArray<voxel::Voxel> results;
		auto collect = [&](int x, int y, int z, const voxel::Voxel &voxel) {
			const glm::ivec3 pos(x, y, z);
			const float factor = blendFactor(pos, center, falloffRadius, _blendOpacity);
			if (factor <= 0.0f) {
				return;
			}
			voxel::Voxel source = voxel;
			if (trackStroke) {
				BlendStrokeSample sample;
				if (_blendStroke.get(pos, sample)) {
					if (factor <= sample.factor) {
						return;
					}
					// Stronger coverage: re-blend from the color at first contact.
					source = sample.original;
					sample.factor = factor;
					_blendStroke.put(pos, sample);
				} else {
					sample.original = voxel;
					sample.factor = factor;
					_blendStroke.put(pos, sample);
				}
			}
			positions.emplace_back(pos);
			if (_paintMode == PaintMode::Blur) {
				results.emplace_back(gaussianBlurVoxel(wrapper, palette, x, y, z, source, factor));
			} else {
				results.emplace_back(mixBlend(palette, source, ctx.cursorVoxel, factor));
			}
		};
		// Sequential visit: the stroke map is not thread-safe, and blur reads neighbors.
		voxelutil::visitVolume(wrapper, region, collect);
		for (size_t i = 0; i < positions.size(); ++i) {
			const glm::ivec3 &pos = positions[i];
			wrapper.setVoxel(pos.x, pos.y, pos.z, results[i]);
		}
		return;
	}

	VoxelColor voxelColor(wrapper.node().palette(), ctx.cursorVoxel, _paintMode, _strength, _variationChance);
	if (floodFill()) {
		voxelutil::paintPlane(wrapper, region.getLowerCorner(), ctx.cursorFace, ctx.hitCursorVoxel,
							  voxelColor.evaluate(ctx.hitCursorVoxel));
	} else if (gradient()) {
		const glm::ivec3 start = ctx.cursorPosition;
		const glm::ivec3 size = region.getDimensionsInVoxels();
		auto func = [&](int x, int y, int z, const voxel::Voxel &voxel) {
			const float factor = glm::distance(glm::vec3(x, y, z), glm::vec3(start)) / glm::length(glm::vec3(size));
			const voxel::Voxel evalVoxel = voxelColor.evaluate(voxel);
			const voxel::Voxel newVoxel = mix(wrapper, ctx.hitCursorVoxel, evalVoxel, factor);
			wrapper.setVoxel(x, y, z, newVoxel);
		};
		voxelutil::visitVolumeParallel(wrapper, region, func);
	} else {
		auto func = [&](int x, int y, int z, const voxel::Voxel &voxel) {
			wrapper.setVoxel(x, y, z, voxelColor.evaluate(voxel));
		};
		voxelutil::visitVolumeParallel(wrapper, region, func);
	}
}

bool PaintBrush::wantBox() const {
	if (floodFill()) {
		return false;
	}
	return Super::wantBox();
}

void PaintBrush::setStrength(float strength) {
	_strength = glm::clamp(strength, 0.1f, 10.0f);
	markDirty();
}

void PaintBrush::setBlendOpacity(float opacity) {
	_blendOpacity = glm::clamp(opacity, 0.0f, 1.0f);
	markDirty();
}

void PaintBrush::setVariationChance(int variationChance) {
	_variationChance = glm::clamp(variationChance, 2, 20);
	markDirty();
}

} // namespace voxedit
