/**
 * @file
 */

#include "voxel/OctreeVolume.h"
#include "render/Shadow.h"
#include "ShaderAttribute.h"
#include "render/RandomColorTexture.h"
#include "video/Camera.h"
#include "VoxelrenderShaders.h"
#include "RenderShaders.h"
#include "video/Shader.h"
#include "video/VertexBuffer.h"
#include "video/UniformBuffer.h"
#include "video/Texture.h"
#include "core/Color.h"

namespace voxelrender {

class OctreeRenderer {
private:
	class RenderOctreeNode {
	public:
		RenderOctreeNode(const video::Shader& shader);
		~RenderOctreeNode();

		video::VertexBuffer _vb;
		video::Id _indexBuffer;
		video::Id _vertexBuffer;

		math::AABB<float> _aabb{glm::zero<glm::vec3>(), glm::zero<glm::vec3>()};

		voxel::TimeStamp _structureLastSynced = 0;
		voxel::TimeStamp _propertiesLastSynced = 0;
		voxel::TimeStamp _meshLastSynced = 0;
		voxel::TimeStamp _nodeAndChildrenLastSynced = 0;

		bool _renderThisNode = false;

		RenderOctreeNode* _children[2][2][2];
	};

	RenderOctreeNode *_rootNode = nullptr;
	voxel::OctreeVolume* _volume = nullptr;

	shader::Materialblock _materialBlock;
	shader::WorldShader _worldShader;
	shader::WorldInstancedShader _worldInstancedShader;
	shader::WaterShader _waterShader;

	glm::vec4 _clearColor = core::Color::LightBlue;
	glm::vec3 _diffuseColor = glm::vec3(1.0, 1.0, 1.0);
	glm::vec3 _ambientColor = glm::vec3(0.2, 0.2, 0.2);
	float _fogRange = 250.0f;
	render::Shadow _shadow;
	render::RandomColorTexture _colorTexture;

	void processOctreeNodeStructure(voxel::OctreeNode* octreeNode, RenderOctreeNode* openGLOctreeNode);
	void renderOctreeNode(const video::Camera& camera, RenderOctreeNode* openGLOctreeNode);

public:
	bool init(voxel::PagedVolume* volume, const voxel::Region& region, int baseNodeSize = 32);
	int update(long dt, const video::Camera& camera);
	void shutdown();

	void render(const video::Camera& camera);
};

}
