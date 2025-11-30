/**
 * @file
 */
#include "TestGLSLGeom.h"
#include "testcore/TestAppMain.h"
#include "color/Color.h"
#include "video/Camera.h"
#include "core/Log.h"

TestGLSLGeom::TestGLSLGeom(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, timeProvider) {
	init(ORGANISATION, "testglslgeom");
}

app::AppState TestGLSLGeom::onInit() {
	app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}

	if (!_testShader.setup()) {
		Log::error("Failed to init the geometry shader");
		return app::AppState::InitFailure;
	}

	_testShader.recordUsedUniforms(true);
	shader::TestData::GeomData geom;
	if (!_testData.create(geom)) {
		Log::error("Failed to init the geometry shader uniform buffer");
		return app::AppState::InitFailure;
	}
	shader::TestData::VertData vert;
	if (!_testData.create(vert)) {
		Log::error("Failed to init the vertex shader uniform buffer");
		return app::AppState::InitFailure;
	}

	struct Buf {
		glm::vec4 pos{0, 0, 0, 1};
		glm::vec3 color{1, 1, 1};
	};
	alignas(32) Buf buf;
	const int32_t bufIndex = _buffer.create(&buf, sizeof(buf));
	_buffer.setMode(bufIndex, video::BufferMode::Static);

	_buffer.addAttribute(_testShader.getPosAttribute(bufIndex, &Buf::pos));
	_buffer.addAttribute(_testShader.getColorAttribute(bufIndex, &Buf::color));

	video::clearColor(color::Color::Black());
	return state;
}

app::AppState TestGLSLGeom::onCleanup() {
	_testShader.shutdown();
	_buffer.shutdown();
	_testData.shutdown();
	return Super::onCleanup();
}

void TestGLSLGeom::onRenderUI() {
	ImGui::SliderFloat("radius", &_radius, 1.0f, 20.0f);
	ImGui::SliderInt("sides", &_sides, 2, _testShader.getMaxGeometryVertices() - 1);
	Super::onRenderUI();
}

void TestGLSLGeom::doRender() {
	video::ScopedShader scopedShd(_testShader);
	shader::TestData::GeomData geom;
	geom.sides = _sides;
	geom.radius = _radius;
	geom.projection = camera().projectionMatrix();
	shader::TestData::VertData vert;
	vert.view = camera().viewMatrix();
	_testData.update(geom);
	_testData.update(vert);
	_testShader.setGeom(_testData.getGeomUniformBuffer());
	_testShader.setVert(_testData.getVertUniformBuffer());
	video::ScopedBuffer scopedBuf(_buffer);
	const int elements = _buffer.elements(0, _testShader.getComponentsPos());
	video::drawArrays(_testShader.getPrimitiveTypeIn(), elements);
}

TEST_APP(TestGLSLGeom)
