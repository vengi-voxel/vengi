/**
 * @file
 */
#include "TestComputeTexture3D.h"
#include "core/io/Filesystem.h"
#include "voxel/MaterialColor.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/VolumeVisitor.h"
#include "voxelgenerator/NoiseGenerator.h"
#include "video/ScopedViewPort.h"

TestComputeTexture3D::TestComputeTexture3D(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider), _renderShader(compute::RenderShader::getInstance()) {
	init(ORGANISATION, "testcomputetexture3d");
}

core::AppState TestComputeTexture3D::onInit() {
	core::AppState state = Super::onInit();
	if (state != core::AppState::Running) {
		return state;
	}

	_camera.setMode(video::CameraMode::Orthogonal);
	_camera.setNearPlane(-1.0f);
	_camera.setFarPlane(1.0f);

	if (!compute::init()) {
		Log::error("Failed to initialize the compute context");
		return core::AppState::InitFailure;
	}

	if (!_renderShader.setup()) {
		Log::error("Failed to setup the compute shader");
		return core::AppState::InitFailure;
	}

	if (!_renderer.init(frameBufferDimension())) {
		Log::error("Failed to setup the renderer");
		return core::AppState::InitFailure;
	}

	if (!voxel::initDefaultMaterialColors()) {
		Log::error("Failed to initialize the palette data");
		return core::AppState::InitFailure;
	}

	initVolume();

	video::clearColor(::core::Color::White);

	return state;
}

void TestComputeTexture3D::initVolume() {
	int width = _workSize.x;
	int height = _workSize.y;
	_output.resize(_workSize.x * _workSize.y * 4);
	_output.shrink_to_fit();

	voxel::Region region(0, 0, 0, width - 1, height - 1, _depth - 1);
	_volume = std::make_shared<voxel::RawVolume>(region);
	math::Random random;
	voxel::RawVolumeWrapper wrapper(_volume.get());
	voxelgenerator::noise::generate(wrapper, 4, 2.0, 0.01, 0.5, voxelgenerator::noise::NoiseType::ridgedMF, random);
	const int amount = voxel::visitVolume(*_volume.get(), [] (int x, int y, int z, const voxel::Voxel& voxel) {
	});
	Log::info("%i voxels", amount);

	if (_texture3DCompute) {
		_texture3DCompute->shutdown();
	}
	compute::TextureConfig cfg3d;
	// the voxel size is two bytes, one byte is for the type the other one is the palette color index
	cfg3d.type(compute::TextureType::Texture3D).format(compute::TextureFormat::RG).dataformat(compute::TextureDataFormat::UNSIGNED_INT8);
	static_assert(sizeof(voxel::Voxel) == 2, "Texture type must be changed if the voxel size is not 16 bits anymore");
	_texture3DCompute = std::make_shared<compute::Texture>(cfg3d, glm::ivec3(width, height, _depth), "volume");
	if (!_texture3DCompute->upload(_volume->data())) {
		Log::error("Failed to upload volume data");
	}

	if (_texture2D) {
		_texture2D->shutdown();
	}
	video::TextureConfig cfg2d;
	cfg2d.type(video::TextureType::Texture2D).format(video::TextureFormat::RGBA);
	_texture2D = video::createTexture(cfg2d, _workSize.x, _workSize.y);
}

core::AppState TestComputeTexture3D::onCleanup() {
	core::AppState state = Super::onCleanup();
	if (_texture3DCompute) {
		_texture3DCompute->shutdown();
	}
	if (_texture2D) {
		_texture2D->shutdown();
	}
	_renderShader.shutdown();
	_renderer.shutdown();
	_volume = std::shared_ptr<voxel::RawVolume>();
	return state;
}

core::AppState TestComputeTexture3D::onRunning() {
	const float step = 0.01f;
	const bool success = _renderShader.render(*_texture3DCompute, _output, _texture3DCompute->width(), _texture3DCompute->height(), _slice, _workSize);
	if (!success) {
		Log::error("Failed to execute compute shader");
	}
	_slice += step;
	if (_slice > 1.0f) {
		_slice = 0.0f;
	}
	return Super::onRunning();
}

void TestComputeTexture3D::onRenderUI() {
	const glm::ivec3 mins = _volume->mins();
	const glm::ivec3 maxs = _volume->maxs();
	ImGui::Text("Slice: %f, # region: %i:%i:%i - %i:%i:%i", _slice, mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
	for (int i = 0; i < _workSize.x * _workSize.y * 4; ++i) {
		// skip alpha value - always 255 - see render.cl
		if (i % 4 == 3) {
			core_assert_msg(_output[i] == 255, "Expected to find the value 255 in the alpha channel - but got %i", (int)_output[i]);
			continue;
		}
		if (_output[i] != 0) {
			ImGui::Text("value i(%i): %i", i, (int)_output[i]);
		}
	}
	if (ImGui::InputInt2("WorkSize", glm::value_ptr(_workSize))) {
		_workSize.x = glm::clamp(_workSize.x, 2, 64);
		_workSize.y = glm::clamp(_workSize.y, 2, 64);
		initVolume();
	}
	if (ImGui::InputInt("Depth", &_depth)) {
		_depth = glm::clamp(_depth, 2, 64);
		initVolume();
	}
	bool temp = _renderTracing;
	if (ImGui::Checkbox("Toggle profiler", &temp)) {
		_renderTracing = toggleTrace();
	}
	ImGui::Separator();
	if (ImGui::Button("Quit")) {
		requestQuit();
	}
}

void TestComputeTexture3D::doRender() {
	_texture2D->upload(_output.data());

	video::ScopedTexture texture(_texture2D, video::TextureUnit::Zero);
	video::ScopedViewPort viewPort(0, 0, frameBufferDimension().x, frameBufferDimension().y);
	_renderer.render(_camera.projectionMatrix());
}

TEST_APP(TestComputeTexture3D)
