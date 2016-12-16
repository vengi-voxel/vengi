/**
 * @file
 */

#pragma once

#include "testcore/TestApp.h"
#include "video/DepthBuffer.h"
#include "video/MeshPool.h"
#include "video/VertexBuffer.h"
#include "FrontendShaders.h"

class TestMeshApp: public TestApp {
private:
	using Super = TestApp;
protected:
	video::DepthBuffer _depthBuffer;
	video::MeshPtr _mesh;
	video::MeshPool _meshPool;
	shader::MeshShader _meshShader;
	shader::ColorShader& _colorShader;
	shader::ShadowmapShader _shadowMapShader;
	glm::vec3 _diffuseColor = glm::vec3(1.0, 1.0, 1.0);
	glm::vec3 _ambientColor = glm::vec3(0.2, 0.2, 0.2);

	virtual void renderPlane();
	virtual void doRender() override;
public:
	TestMeshApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	virtual core::AppState onConstruct() override;
	virtual core::AppState onInit() override;
	virtual core::AppState onCleanup() override;
};
