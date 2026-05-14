/**
 * @file
 */

#pragma once

#include "color/RGBA.h"
#include "core/IComponent.h"
#include "core/SharedPtr.h"
#include "core/Trace.h"
#include "core/collection/DynamicArray.h"
#include "core/concurrent/Lock.h"
#include "math/Axis.h"
#include "voxel/Face.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

namespace palette {
class Palette;
}

namespace voxel {
class RawVolume;
}

namespace video {
class Camera;
}

namespace voxelrender {
struct RenderContext;
}

namespace voxedit {

/**
 * @brief Context passed to the modifier renderer for updating and rendering state.
 *
 * This aggregates all the information the renderer needs to update its internal buffers
 * and render the modifier visuals. The renderer implementation handles all buffer updates
 * internally based on this context.
 */
struct ModifierRendererContext {
	// Cursor info
	voxel::Voxel cursorVoxel;
	voxel::Voxel voxelAtCursor;
	voxel::FaceNames cursorFace = voxel::FaceNames::Max;
	glm::ivec3 cursorPosition{0};
	int gridResolution = 1;

	// Reference point
	glm::ivec3 referencePosition{0};

	// Mirror plane
	math::Axis mirrorAxis = math::Axis::None;
	glm::ivec3 mirrorPos{0};

	// Locked planes
	math::Axis lockedAxis = math::Axis::None;

	voxel::Region activeRegion;

	// Brush preview
	voxel::RawVolume *previewVolume = nullptr;
	voxel::RawVolume *previewMirrorVolume = nullptr;
	voxel::Region simplePreviewRegion;
	voxel::Region simpleMirrorPreviewRegion;
	color::RGBA simplePreviewColor{0};
	palette::Palette *palette = nullptr;
	bool useSimplePreview = false;
	bool brushActive = false;
};

class IModifierRenderer : public core::IComponent {
protected:
	enum class CommandType : uint8_t { HighlightRegion };

	struct CommandEvent {
		CommandType type;
		union {
			struct {
				int32_t regionMins[3];
				int32_t regionMaxs[3];
				uint64_t renderRegionMillis;
			} highlightRegion;
		};
	};

	core_trace_mutex(core::Lock, _commandBufferMutex, "IModifierRenderer");
	core::DynamicArray<CommandEvent> _commandBuffer;

	core::DynamicArray<CommandEvent> popCommandBuffer() {
		core::DynamicArray<CommandEvent> cmds;
		{
			core::ScopedLock lock(_commandBufferMutex);
			cmds = core::move(_commandBuffer);
		}
		return cmds;
	}

public:
	bool init() override {
		return true;
	}
	void shutdown() override {
	}

	/**
	 * @brief Update all internal renderer state based on the given context.
	 *
	 * The renderer implementation should handle:
	 * - Cursor visualization updates
	 * - Mirror plane updates
	 * - Selection buffer updates
	 * - Brush preview volume mesh updates
	 *
	 * @param ctx The context containing all necessary state for the update.
	 */
	virtual void update(const ModifierRendererContext &ctx) {
	}

	virtual void setHighlightRegion(const voxel::Region &region, uint64_t renderRegionMillis = 0) {
		CommandEvent cmd;
		cmd.type = CommandType::HighlightRegion;
		cmd.highlightRegion.regionMins[0] = region.getLowerX();
		cmd.highlightRegion.regionMins[1] = region.getLowerY();
		cmd.highlightRegion.regionMins[2] = region.getLowerZ();
		cmd.highlightRegion.regionMaxs[0] = region.getUpperX();
		cmd.highlightRegion.regionMaxs[1] = region.getUpperY();
		cmd.highlightRegion.regionMaxs[2] = region.getUpperZ();
		cmd.highlightRegion.renderRegionMillis = renderRegionMillis;
		core::ScopedLock lock(_commandBufferMutex);
		_commandBuffer.push_back(cmd);
	}

	/**
	 * @brief Clear the renderer's brush volume pointers to nullptr.
	 *
	 * Must be called before freeing preview volumes to prevent dangling pointer
	 * comparisons in MeshState::setVolume(). Without this, the allocator may reuse
	 * the same address for a new volume, causing setVolume() to skip the update
	 * (old == new pointer) and leaving stale mesh data in GPU buffers.
	 */
	virtual void clearBrushVolumes() {
	}

	/**
	 * @brief Wait for any pending mesh extractions to complete.
	 *
	 * This must be called before freeing preview volumes to avoid race conditions
	 * where extraction threads are still reading from a volume that gets freed.
	 */
	virtual void waitForPendingExtractions() {
	}

	/**
	 * @brief Render all modifier visuals.
	 *
	 * @param camera The camera for rendering.
	 * @param modelMatrix The model transformation matrix.
	 */
	virtual void render(voxelrender::RenderContext &renderContext, const video::Camera &camera, const glm::mat4 &modelMatrix) {
	}
};

using ModifierRendererPtr = core::SharedPtr<IModifierRenderer>;

} // namespace voxedit
