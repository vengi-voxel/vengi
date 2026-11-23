/**
 * @file
 */

#pragma once

#include "video/Camera.h"
#include "color/Color.h"
#include "math/Plane.h"
#include "video/ShapeBuilder.h"
#include "render/ShapeRenderer.h"
#include "core/IComponent.h"
#include "math/Mat4f.h"
#include "math/Vec4f.h"
#include "math/Vec3f.h"

namespace video {
class Video;
}

namespace render {

/**
 * @brief Renders a plane
 *
 * @see video::ShapeBuilder
 * @see ShapeRenderer
 */
class Plane : public core::IComponent {
private:
	video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;
	bool _planeMeshes[render::ShapeRenderer::MAX_MESHES] { false };
public:
	void render(const video::Camera& camera, const math::Mat4f& model = math::Mat4f(1.0f)) const;

	void shutdown() override;

	void clear();

	bool init() override;

	/**
	 * @param[in] position The offset that should be applied to the center of the plane
	 * @param[in] tesselation The amount of splits on the plane that should be made
	 * @param[in] color The color of the plane.
	 */
	bool plane(const math::Vec3f& position, int tesselation = 0, const math::Vec4f& color = color::White());

	bool plane(const math::Vec3f& position, const math::Plane& plane, const math::Vec4f& color = color::White());
};

}
