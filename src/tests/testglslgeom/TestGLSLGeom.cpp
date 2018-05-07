/**
 * @file
 */
#include "TestGLSLGeom.h"
#include "io/Filesystem.h"
#include "core/Color.h"
#include "video/Camera.h"
#include "video/ScopedViewPort.h"

TestGLSLGeom::TestGLSLGeom(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "testglslgeom");
}

core::AppState TestGLSLGeom::onInit() {
	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	if (!_testShader.setup()) {
		Log::error("Failed to init the geometry shader");
		return core::AppState::InitFailure;
	}

	_testShader.recordUsedUniforms(true);

	struct Buf {
		glm::vec4 pos{0, 0, 0, 1};
		glm::vec3 color{1, 1, 1};
	};
	Buf buf;
	int32_t bufIndex = _buffer.create(&buf, sizeof(buf));
	_buffer.setMode(bufIndex, video::BufferMode::Static);

	video::Attribute attributePos;
	attributePos.bufferIndex = bufIndex;
	attributePos.index = _testShader.getLocationPos();
	attributePos.size = _testShader.getComponentsPos();
	attributePos.offset = offsetof(Buf, pos);
	attributePos.stride = sizeof(Buf);
	_buffer.addAttribute(attributePos);

	video::Attribute attributeColor;
	attributeColor.bufferIndex = bufIndex;
	attributeColor.index = _testShader.getLocationColor();
	attributeColor.size = _testShader.getComponentsColor();
	attributeColor.offset = offsetof(Buf, color);
	attributeColor.stride = sizeof(Buf);
	_buffer.addAttribute(attributeColor);

	video::clearColor(::core::Color::Black);
	return state;
}

core::AppState TestGLSLGeom::onCleanup() {
	core::AppState state = Super::onCleanup();
	_testShader.shutdown();
	_buffer.shutdown();
	return state;
}

void TestGLSLGeom::onRenderUI() {
	ImGui::SliderFloat("radius", &_radius, 1.0f, 20.0f);
	ImGui::SliderInt("sides", &_sides, 2, _testShader.getMaxGeometryVertices() - 1);
	Super::onRenderUI();
}

void TestGLSLGeom::doRender() {
	video::ScopedShader scopedShd(_testShader);
	_testShader.setSides(_sides);
	_testShader.setRadius(_radius);
	_testShader.setView(_camera.viewMatrix());
	_testShader.setProjection(_camera.projectionMatrix());
	video::ScopedBuffer scopedBuf(_buffer);
	const int elements = _buffer.elements(0, _testShader.getComponentsPos());
	video::drawArrays(_testShader.getPrimitiveTypeIn(), elements);
}

TEST_APP(TestGLSLGeom)
