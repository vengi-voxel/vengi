/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "video/DepthBuffer.h"
#include "video/MeshPool.h"
#include "video/VertexBuffer.h"
#include "frontend/Shadow.h"
#include "FrontendShaders.h"

/**
 * @brief Application that is able to render meshes
 */
class TestMeshApp: public TestApp {
private:
	using Super = TestApp;
protected:
	video::VertexBuffer _shadowMapDebugBuffer;
	video::DepthBuffer _depthBuffer;
	video::MeshPtr _mesh;
	video::MeshPool _meshPool;
	frontend::Shadow _shadow;
	shader::MeshShader _meshShader;
	shader::ColorShader& _colorShader;
	shader::ShadowmapShader _shadowMapShader;
	shader::ShadowmapRenderShader _shadowMapRenderShader;
	glm::vec3 _position {0.0f, 0.0f, 0.0f};
	glm::vec3 _diffuseColor {1.0, 1.0, 1.0};
	glm::vec3 _ambientColor {0.2, 0.2, 0.2};
	glm::vec4 _fogColor;
	glm::vec4 _clearColor {0.8f, 0.8f, 0.8f, 1.0f};
	glm::vec3 _omega { 0.0f };
	bool _renderNormals = true;
	float _fogRange = 250.0f;
	float _shadowBiasSlope = 2.0f;
	float _shadowBias = 0.09f;
	float _shadowRangeZ = 0.0f;
	int _boneInfluence = 0;
	glm::mat4 _model {1.0f};
	core::VarPtr _shadowMapShow;
	core::VarPtr _shadowMap;
	core::VarPtr _animationIndex;
	core::VarPtr _meshName;
	core::VarPtr _debugShadow;
	core::VarPtr _debugShadowCascade;

	virtual void renderPlane(video::Shader* shader = nullptr);
	virtual void doRender() override;
	virtual void onRenderUI() override;
public:
	TestMeshApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	virtual core::AppState onConstruct() override;
	virtual core::AppState onInit() override;
	virtual core::AppState onCleanup() override;
};
