/**
 * @file
 */

#include "Modifier.h"
#include "ModifierRenderer.h"
#include "core/IComponent.h"

namespace voxedit {

class ModifierFacade : public Modifier  {
private:
	using Super = Modifier;
	ModifierRenderer _modifierRenderer;

protected:
	bool select(const glm::ivec3& mins, const glm::ivec3& maxs) override;
	void unselect() override;

public:
	bool init() override;
	void shutdown() override;

	void setCursorVoxel(const voxel::Voxel& voxel) override;
	bool setMirrorAxis(math::Axis axis, const glm::ivec3 &mirrorPos) override;
	void render(const video::Camera& camera);
};

}
